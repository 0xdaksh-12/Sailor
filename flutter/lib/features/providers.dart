import 'dart:io';
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:path/path.dart' as p;
import 'package:shared_preferences/shared_preferences.dart';
import '../core/models/models.dart';
import '../core/sailor_client.dart';

// Client Provider
final clientProvider = Provider<SailorClient>((ref) {
  final client = SailorClient();
  client.init();
  ref.onDispose(() => client.dispose());
  return client;
});

// Connection Notifier & Provider
class ConnectionNotifier extends StateNotifier<ConnectionStateData> {
  final SailorClient _client;
  final Ref _ref;

  ConnectionNotifier(this._client, this._ref) : super(const ConnectionStateData());

  Future<bool> connect({
    required String host,
    required int port,
    required String username,
    required String password,
  }) async {
    state = state.copyWith(error: null);
    try {
      await _client.connect(host, port, username, password);
      state = ConnectionStateData(
        isConnected: true,
        host: host,
        port: port,
        username: username,
        connectedAt: DateTime.now(),
      );

      // Initialize remote browser at root
      await _ref.read(remoteBrowserProvider.notifier).loadPath('/');
      return true;
    } catch (e) {
      state = state.copyWith(isConnected: false, error: e.toString());
      return false;
    }
  }

  Future<void> disconnect() async {
    try {
      await _client.disconnect();
    } catch (_) {}
    state = const ConnectionStateData(isConnected: false);
    _ref.read(remoteBrowserProvider.notifier).reset();
  }
}

final connectionProvider = StateNotifierProvider<ConnectionNotifier, ConnectionStateData>((ref) {
  return ConnectionNotifier(ref.watch(clientProvider), ref);
});

// Remote Browser State & Notifier
class RemoteBrowserState {
  final String currentPath;
  final List<FileEntry> entries;
  final bool isLoading;
  final String? error;

  const RemoteBrowserState({
    this.currentPath = '/',
    this.entries = const [],
    this.isLoading = false,
    this.error,
  });

  RemoteBrowserState copyWith({
    String? currentPath,
    List<FileEntry>? entries,
    bool? isLoading,
    String? error,
  }) {
    return RemoteBrowserState(
      currentPath: currentPath ?? this.currentPath,
      entries: entries ?? this.entries,
      isLoading: isLoading ?? this.isLoading,
      error: error,
    );
  }
}

class RemoteBrowserNotifier extends StateNotifier<RemoteBrowserState> {
  final SailorClient _client;

  RemoteBrowserNotifier(this._client) : super(const RemoteBrowserState());

  void reset() {
    state = const RemoteBrowserState();
  }

  String normalizeRemotePath(String path) {
    if (path.isEmpty || path == '/') return '/';
    final parts = path.split('/').where((s) => s.isNotEmpty && s != '.').toList();
    final stack = <String>[];
    for (final p in parts) {
      if (p == '..') {
        if (stack.isNotEmpty) stack.removeLast();
      } else {
        stack.add(p);
      }
    }
    return '/${stack.join('/')}';
  }

  Future<void> loadPath(String path) async {
    final normalized = normalizeRemotePath(path);
    state = state.copyWith(currentPath: normalized, isLoading: true, error: null);

    try {
      final items = await _client.list(normalized);
      items.sort((a, b) {
        if (a.isDirectory != b.isDirectory) {
          return a.isDirectory ? -1 : 1;
        }
        return a.name.toLowerCase().compareTo(b.name.toLowerCase());
      });
      state = state.copyWith(currentPath: normalized, entries: items, isLoading: false);
    } catch (e) {
      state = state.copyWith(isLoading: false, error: e.toString());
    }
  }

  void refresh() => loadPath(state.currentPath);

  void navigateUp() {
    if (state.currentPath == '/' || state.currentPath.isEmpty) return;
    final parent = p.posix.dirname(state.currentPath);
    loadPath(parent);
  }

  Future<void> mkdir(String folderName) async {
    final target = state.currentPath == '/' ? '/$folderName' : '${state.currentPath}/$folderName';
    await _client.mkdir(target);
    refresh();
  }

  Future<void> deletePath(FileEntry entry) async {
    final target = state.currentPath == '/' ? '/${entry.name}' : '${state.currentPath}/${entry.name}';
    if (entry.isDirectory) {
      await _client.rmdir(target);
    } else {
      await _client.delete(target);
    }
    refresh();
  }

  Future<void> rename(String oldName, String newName) async {
    final oldPath = state.currentPath == '/' ? '/$oldName' : '${state.currentPath}/$oldName';
    final newPath = state.currentPath == '/' ? '/$newName' : '${state.currentPath}/$newName';
    await _client.rename(oldPath, newPath);
    refresh();
  }
}

final remoteBrowserProvider = StateNotifierProvider<RemoteBrowserNotifier, RemoteBrowserState>((ref) {
  return RemoteBrowserNotifier(ref.watch(clientProvider));
});

// Local Browser State & Notifier
class LocalBrowserState {
  final String currentPath;
  final List<FileEntry> entries;
  final bool isLoading;
  final String? error;

  const LocalBrowserState({
    required this.currentPath,
    this.entries = const [],
    this.isLoading = false,
    this.error,
  });

  LocalBrowserState copyWith({
    String? currentPath,
    List<FileEntry>? entries,
    bool? isLoading,
    String? error,
  }) {
    return LocalBrowserState(
      currentPath: currentPath ?? this.currentPath,
      entries: entries ?? this.entries,
      isLoading: isLoading ?? this.isLoading,
      error: error,
    );
  }
}

class LocalBrowserNotifier extends StateNotifier<LocalBrowserState> {
  LocalBrowserNotifier()
      : super(LocalBrowserState(
          currentPath: Platform.environment['HOME'] ?? Directory.current.path,
        )) {
    loadPath(state.currentPath);
  }

  void loadPath(String path) {
    try {
      final dir = Directory(path);
      if (!dir.existsSync()) {
        state = state.copyWith(error: 'Directory does not exist');
        return;
      }

      final entities = dir.listSync(followLinks: false);
      final entries = <FileEntry>[];

      for (final e in entities) {
        try {
          final stat = e.statSync();
          final name = p.basename(e.path);
          if (name.startsWith('.')) continue; // skip hidden files by default
          entries.add(FileEntry(
            name: name,
            isDirectory: stat.type == FileSystemEntityType.directory,
            size: stat.size,
            modifiedTime: stat.modified,
          ));
        } catch (_) {}
      }

      entries.sort((a, b) {
        if (a.isDirectory != b.isDirectory) {
          return a.isDirectory ? -1 : 1;
        }
        return a.name.toLowerCase().compareTo(b.name.toLowerCase());
      });

      state = LocalBrowserState(
        currentPath: dir.absolute.path,
        entries: entries,
        isLoading: false,
      );
    } catch (e) {
      state = state.copyWith(error: e.toString());
    }
  }

  void refresh() => loadPath(state.currentPath);

  void navigateUp() {
    final parent = Directory(state.currentPath).parent;
    if (parent.existsSync()) {
      loadPath(parent.path);
    }
  }
}

final localBrowserProvider = StateNotifierProvider<LocalBrowserNotifier, LocalBrowserState>((ref) {
  return LocalBrowserNotifier();
});

// Transfer Queue State & Notifier
class TransferNotifier extends StateNotifier<List<TransferTask>> {
  final SailorClient _client;
  final Ref _ref;
  bool _isProcessing = false;

  TransferNotifier(this._client, this._ref) : super([]) {
    _client.progressStream.listen((data) {
      if (state.isEmpty) return;
      final activeIndex = state.indexWhere((t) => t.status == TransferStatus.inProgress);
      if (activeIndex != -1) {
        final task = state[activeIndex];
        task.transferredBytes = (data['sent'] as num?)?.toInt() ?? task.transferredBytes;
        state = [...state];
      }
    });
  }

  Future<void> enqueueUpload(String localPath, String remoteDir) async {
    final file = File(localPath);
    if (!file.existsSync()) return;

    final filename = p.basename(localPath);
    final targetRemote = remoteDir == '/' ? '/$filename' : '$remoteDir/$filename';

    final task = TransferTask(
      id: '${DateTime.now().microsecondsSinceEpoch}_up',
      filename: filename,
      localPath: localPath,
      remotePath: targetRemote,
      totalBytes: file.lengthSync(),
      direction: TransferDirection.upload,
    );

    state = [...state, task];
    _processQueue();
  }

  Future<void> enqueueDownload(String remotePath, String filename, int size, String localDir) async {
    final targetLocal = p.join(localDir, filename);

    final task = TransferTask(
      id: '${DateTime.now().microsecondsSinceEpoch}_down',
      filename: filename,
      localPath: targetLocal,
      remotePath: remotePath,
      totalBytes: size,
      direction: TransferDirection.download,
    );

    state = [...state, task];
    _processQueue();
  }

  void cancelTask(String id) {
    state = state.map((t) {
      if (t.id == id && (t.status == TransferStatus.queued || t.status == TransferStatus.inProgress)) {
        t.status = TransferStatus.cancelled;
      }
      return t;
    }).toList();
  }

  void clearCompleted() {
    state = state.where((t) => t.status != TransferStatus.completed && t.status != TransferStatus.cancelled).toList();
  }

  void retryTask(String id) {
    state = state.map((t) {
      if (t.id == id && t.status == TransferStatus.failed) {
        t.status = TransferStatus.queued;
        t.errorMessage = null;
        t.transferredBytes = 0;
      }
      return t;
    }).toList();
    _processQueue();
  }

  Future<void> _processQueue() async {
    if (_isProcessing) return;
    _isProcessing = true;

    try {
      while (state.any((t) => t.status == TransferStatus.queued)) {
        final taskIndex = state.indexWhere((t) => t.status == TransferStatus.queued);
        if (taskIndex == -1) break;

        final task = state[taskIndex];
        task.status = TransferStatus.inProgress;
        task.startTime = DateTime.now();
        state = [...state];

        try {
          if (task.direction == TransferDirection.upload) {
            final remoteDir = p.posix.dirname(task.remotePath);
            await _client.upload(task.localPath, remoteDir.isEmpty ? '/' : remoteDir);
          } else {
            final localParent = p.dirname(task.localPath);
            if (!Directory(localParent).existsSync()) {
              Directory(localParent).createSync(recursive: true);
            }
            await _client.download(task.remotePath, task.localPath);
          }
          task.transferredBytes = task.totalBytes;
          task.status = TransferStatus.completed;
          task.endTime = DateTime.now();
        } catch (e) {
          task.status = TransferStatus.failed;
          task.errorMessage = e.toString();
          task.endTime = DateTime.now();
        }

        state = [...state];

        // Refresh relevant panes
        _ref.read(remoteBrowserProvider.notifier).refresh();
        _ref.read(localBrowserProvider.notifier).refresh();
      }
    } finally {
      _isProcessing = false;
    }
  }
}

final transferQueueProvider = StateNotifierProvider<TransferNotifier, List<TransferTask>>((ref) {
  return TransferNotifier(ref.watch(clientProvider), ref);
});

// Settings State & Notifier
class SettingsState {
  final ThemeMode themeMode;
  final String defaultDownloadDir;
  final String defaultHost;
  final int defaultPort;
  final String defaultUsername;

  const SettingsState({
    this.themeMode = ThemeMode.dark,
    this.defaultDownloadDir = '',
    this.defaultHost = '127.0.0.1',
    this.defaultPort = 9000,
    this.defaultUsername = 'admin',
  });

  SettingsState copyWith({
    ThemeMode? themeMode,
    String? defaultDownloadDir,
    String? defaultHost,
    int? defaultPort,
    String? defaultUsername,
  }) {
    return SettingsState(
      themeMode: themeMode ?? this.themeMode,
      defaultDownloadDir: defaultDownloadDir ?? this.defaultDownloadDir,
      defaultHost: defaultHost ?? this.defaultHost,
      defaultPort: defaultPort ?? this.defaultPort,
      defaultUsername: defaultUsername ?? this.defaultUsername,
    );
  }
}

class SettingsNotifier extends StateNotifier<SettingsState> {
  SettingsNotifier() : super(const SettingsState()) {
    _loadSettings();
  }

  Future<void> _loadSettings() async {
    final prefs = await SharedPreferences.getInstance();
    final themeIndex = prefs.getInt('theme_mode') ?? 0;
    final homeDir = Platform.environment['HOME'] ?? Directory.current.path;
    final downloadDir = prefs.getString('download_dir') ?? p.join(homeDir, 'Downloads');
    final host = prefs.getString('default_host') ?? '127.0.0.1';
    final port = prefs.getInt('default_port') ?? 9000;
    final username = prefs.getString('default_username') ?? 'admin';

    state = SettingsState(
      themeMode: ThemeMode.values[themeIndex.clamp(0, ThemeMode.values.length - 1)],
      defaultDownloadDir: downloadDir,
      defaultHost: host,
      defaultPort: port,
      defaultUsername: username,
    );
  }

  Future<void> updateTheme(ThemeMode mode) async {
    state = state.copyWith(themeMode: mode);
    final prefs = await SharedPreferences.getInstance();
    await prefs.setInt('theme_mode', mode.index);
  }

  Future<void> updateDefaults({
    String? downloadDir,
    String? host,
    int? port,
    String? username,
  }) async {
    state = state.copyWith(
      defaultDownloadDir: downloadDir,
      defaultHost: host,
      defaultPort: port,
      defaultUsername: username,
    );
    final prefs = await SharedPreferences.getInstance();
    if (downloadDir != null) await prefs.setString('download_dir', downloadDir);
    if (host != null) await prefs.setString('default_host', host);
    if (port != null) await prefs.setInt('default_port', port);
    if (username != null) await prefs.setString('default_username', username);
  }
}

final settingsProvider = StateNotifierProvider<SettingsNotifier, SettingsState>((ref) {
  return SettingsNotifier();
});
