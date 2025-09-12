import 'dart:async';
import 'dart:ffi';
import 'dart:io';
import 'dart:isolate';
import 'package:ffi/ffi.dart';
import 'package:path/path.dart' as p;
import 'ffi_bindings.dart';
import 'models/models.dart';

class SailorClientException implements Exception {
  final String message;
  final int? code;

  SailorClientException(this.message, [this.code]);

  @override
  String toString() => message;
}

class SailorClient {
  final String? customLibraryPath;
  SendPort? _commandPort;
  Isolate? _workerIsolate;
  bool _isInitialized = false;

  final _progressStreamController = StreamController<Map<String, dynamic>>.broadcast();
  Stream<Map<String, dynamic>> get progressStream => _progressStreamController.stream;

  SailorClient({this.customLibraryPath});

  static String resolveLibraryPath(String? customPath) {
    if (customPath != null && File(customPath).existsSync()) {
      return File(customPath).absolute.path;
    }

    final currentDir = Directory.current.absolute.path;
    final candidates = [
      'libsailor.so',
      p.join(currentDir, 'build', 'libsailor.so'),
      p.join(currentDir, '..', 'build', 'libsailor.so'),
      p.join(p.dirname(Platform.resolvedExecutable), 'libsailor.so'),
      p.join(p.dirname(Platform.resolvedExecutable), 'lib', 'libsailor.so'),
      '/home/daksh/Desktop/FUCK/Sailor/build/libsailor.so',
      '/usr/local/lib/libsailor.so',
      '/usr/lib/libsailor.so',
    ];

    for (final candidate in candidates) {
      if (File(candidate).existsSync()) {
        return File(candidate).absolute.path;
      }
    }

    return customPath ?? 'libsailor.so';
  }

  Future<void> init() async {
    if (_isInitialized) return;

    final resolvedLibPath = resolveLibraryPath(customLibraryPath);
    final initPort = ReceivePort();
    _workerIsolate = await Isolate.spawn(_workerEntry, [initPort.sendPort, resolvedLibPath]);

    final commandPortCompleter = Completer<SendPort>();

    initPort.listen((message) {
      if (message is SendPort) {
        if (!commandPortCompleter.isCompleted) {
          commandPortCompleter.complete(message);
        }
      } else if (message is Map<String, dynamic>) {
        if (message['type'] == 'progress') {
          _progressStreamController.add(message);
        }
      }
    });

    _commandPort = await commandPortCompleter.future;
    _isInitialized = true;
  }

  Future<T> _sendRequest<T>(String action, [Map<String, dynamic>? args]) async {
    if (!_isInitialized || _commandPort == null) {
      await init();
    }

    final responsePort = ReceivePort();
    _commandPort!.send({'action': action, 'args': args, 'port': responsePort.sendPort});
    final res = await responsePort.first;
    if (res is Map && res['error'] != null) {
      final code = res['code'] as int?;
      throw SailorClientException(res['error'].toString(), code);
    }
    return res['data'] as T;
  }

  Future<void> connect(String host, int port, String user, String pass) =>
      _sendRequest<void>('connect', {'host': host, 'port': port, 'user': user, 'pass': pass});

  Future<void> disconnect() => _sendRequest<void>('disconnect');

  Future<bool> isConnected() => _sendRequest<bool>('isConnected');

  Future<List<FileEntry>> list(String path) async {
    final list = await _sendRequest<List<dynamic>>('list', {'path': path});
    return list.map((e) => FileEntry(
      name: e['name'] as String,
      isDirectory: e['isDirectory'] as bool,
      size: e['size'] as int,
      modifiedTime: DateTime.fromMillisecondsSinceEpoch(e['mtime'] as int),
    )).toList();
  }

  Future<void> upload(String localPath, String remoteDir) =>
      _sendRequest<void>('upload', {'local': localPath, 'remote': remoteDir});

  Future<void> download(String remotePath, String localPath) =>
      _sendRequest<void>('download', {'remote': remotePath, 'local': localPath});

  Future<void> delete(String remotePath) =>
      _sendRequest<void>('delete', {'path': remotePath});

  Future<void> rename(String src, String dst) =>
      _sendRequest<void>('rename', {'src': src, 'dst': dst});

  Future<void> mkdir(String path) =>
      _sendRequest<void>('mkdir', {'path': path});

  Future<void> rmdir(String path) =>
      _sendRequest<void>('rmdir', {'path': path});

  void dispose() {
    _workerIsolate?.kill(priority: Isolate.immediate);
    _workerIsolate = null;
    _commandPort = null;
    _isInitialized = false;
    _progressStreamController.close();
  }

  static String _statusToMessage(int code) {
    switch (code) {
      case 0:
        return 'Success';
      case -1:
        return 'Connection failed (server unreachable or network error)';
      case -2:
        return 'Authentication failed (invalid credentials)';
      case -3:
        return 'File or directory not found';
      case -4:
        return 'Permission denied';
      case -5:
        return 'Invalid path or path traversal detected';
      case -6:
        return 'Internal server/client error';
      case -7:
        return 'Not connected to server';
      case -8:
        return 'Operation failed';
      default:
        return 'Error code: $code';
    }
  }

  static void _workerEntry(List<dynamic> initArgs) {
    final SendPort notifyPort = initArgs[0] as SendPort;
    final String libPath = initArgs[1] as String;

    final cmdPort = ReceivePort();
    notifyPort.send(cmdPort.sendPort);

    DynamicLibrary dylib;
    SailorBindings bindings;

    try {
      dylib = DynamicLibrary.open(libPath);
      bindings = SailorBindings(dylib);
    } catch (e) {
      cmdPort.listen((msg) {
        final SendPort respPort = msg['port'] as SendPort;
        respPort.send({'error': 'Failed to load native library $libPath: $e'});
      });
      return;
    }

    final progressCallback = NativeCallable<SailorProgressNative>.listener((int sent, int total) {
      notifyPort.send({'type': 'progress', 'sent': sent, 'total': total});
    });
    bindings.sailor_set_progress_callback(progressCallback.nativeFunction);

    cmdPort.listen((msg) {
      final String action = msg['action'] as String;
      final Map<String, dynamic>? args = msg['args'] as Map<String, dynamic>?;
      final SendPort respPort = msg['port'] as SendPort;

      try {
        switch (action) {
          case 'connect':
            final host = args!['host'] as String;
            final port = args['port'] as int;
            final user = args['user'] as String;
            final pass = args['pass'] as String;

            final h = host.toNativeUtf8();
            final u = user.toNativeUtf8();
            final p = pass.toNativeUtf8();
            try {
              final code = bindings.sailor_connect(h, port, u, p);
              if (code != 0) {
                respPort.send({'error': _statusToMessage(code), 'code': code});
              } else {
                respPort.send({'data': null});
              }
            } finally {
              calloc.free(h);
              calloc.free(u);
              calloc.free(p);
            }
            break;

          case 'disconnect':
            bindings.sailor_disconnect();
            respPort.send({'data': null});
            break;

          case 'isConnected':
            final isConn = bindings.sailor_is_connected() == 1;
            respPort.send({'data': isConn});
            break;

          case 'list':
            final pathStr = args!['path'] as String;
            final pathPtr = pathStr.toNativeUtf8();
            try {
              final resPtr = bindings.sailor_list(pathPtr);
              if (resPtr == nullptr) {
                respPort.send({'error': 'Directory listing failed for "$pathStr"', 'code': -8});
                break;
              }

              final items = <Map<String, dynamic>>[];
              final res = resPtr.ref;
              for (int i = 0; i < res.count; i++) {
                final e = res.entries[i];
                final entryName = e.name == nullptr ? '' : e.name.toDartString();
                items.add({
                  'name': entryName,
                  'isDirectory': e.isDirectory == 1,
                  'size': e.size,
                  'mtime': e.modifiedTime * 1000,
                });
              }
              bindings.sailor_free_list(resPtr);
              respPort.send({'data': items});
            } finally {
              calloc.free(pathPtr);
            }
            break;

          case 'upload':
            final local = args!['local'] as String;
            final remote = args['remote'] as String;
            final lPtr = local.toNativeUtf8();
            final rPtr = remote.toNativeUtf8();
            try {
              final res = bindings.sailor_upload(lPtr, rPtr);
              if (res != 0) {
                respPort.send({'error': _statusToMessage(res), 'code': res});
              } else {
                respPort.send({'data': null});
              }
            } finally {
              calloc.free(lPtr);
              calloc.free(rPtr);
            }
            break;

          case 'download':
            final remote = args!['remote'] as String;
            final local = args['local'] as String;
            final rPtr = remote.toNativeUtf8();
            final lPtr = local.toNativeUtf8();
            try {
              final res = bindings.sailor_download(rPtr, lPtr);
              if (res != 0) {
                respPort.send({'error': _statusToMessage(res), 'code': res});
              } else {
                respPort.send({'data': null});
              }
            } finally {
              calloc.free(rPtr);
              calloc.free(lPtr);
            }
            break;

          case 'delete':
            final path = args!['path'] as String;
            final pPtr = path.toNativeUtf8();
            try {
              final res = bindings.sailor_delete(pPtr);
              if (res != 0) {
                respPort.send({'error': _statusToMessage(res), 'code': res});
              } else {
                respPort.send({'data': null});
              }
            } finally {
              calloc.free(pPtr);
            }
            break;

          case 'rename':
            final src = args!['src'] as String;
            final dst = args['dst'] as String;
            final sPtr = src.toNativeUtf8();
            final dPtr = dst.toNativeUtf8();
            try {
              final res = bindings.sailor_rename(sPtr, dPtr);
              if (res != 0) {
                respPort.send({'error': _statusToMessage(res), 'code': res});
              } else {
                respPort.send({'data': null});
              }
            } finally {
              calloc.free(sPtr);
              calloc.free(dPtr);
            }
            break;

          case 'mkdir':
            final path = args!['path'] as String;
            final pPtr = path.toNativeUtf8();
            try {
              final res = bindings.sailor_mkdir(pPtr);
              if (res != 0) {
                respPort.send({'error': _statusToMessage(res), 'code': res});
              } else {
                respPort.send({'data': null});
              }
            } finally {
              calloc.free(pPtr);
            }
            break;

          case 'rmdir':
            final path = args!['path'] as String;
            final pPtr = path.toNativeUtf8();
            try {
              final res = bindings.sailor_rmdir(pPtr);
              if (res != 0) {
                respPort.send({'error': _statusToMessage(res), 'code': res});
              } else {
                respPort.send({'data': null});
              }
            } finally {
              calloc.free(pPtr);
            }
            break;

          default:
            respPort.send({'error': 'Unknown action: $action'});
        }
      } catch (e) {
        respPort.send({'error': e.toString()});
      }
    });
  }
}
