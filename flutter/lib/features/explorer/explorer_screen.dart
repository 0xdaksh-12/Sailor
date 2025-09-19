import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:file_picker/file_picker.dart';
import 'package:path/path.dart' as p;
import '../../core/models/models.dart';
import '../connect/connect_screen.dart';
import '../providers.dart';
import '../settings/settings_dialog.dart';
import '../transfers/transfer_queue_panel.dart';

class ExplorerScreen extends ConsumerWidget {
  const ExplorerScreen({super.key});

  void _handleDisconnect(BuildContext context, WidgetRef ref) {
    showDialog(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Row(
          children: [
            Icon(Icons.logout, color: Colors.amber),
            SizedBox(width: 8),
            Text('Disconnect'),
          ],
        ),
        content: const Text('Are you sure you want to disconnect from the server?'),
        actions: [
          TextButton(
            onPressed: () => Navigator.of(ctx).pop(),
            child: const Text('Cancel'),
          ),
          FilledButton(
            style: FilledButton.styleFrom(backgroundColor: Colors.redAccent),
            onPressed: () async {
              Navigator.of(ctx).pop();
              await ref.read(connectionProvider.notifier).disconnect();
              if (context.mounted) {
                Navigator.of(context).pushReplacement(
                  MaterialPageRoute(builder: (_) => const ConnectScreen()),
                );
              }
            },
            child: const Text('Disconnect'),
          ),
        ],
      ),
    );
  }

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final conn = ref.watch(connectionProvider);
    final theme = Theme.of(context);

    return Scaffold(
      appBar: AppBar(
        title: Row(
          children: [
            Icon(Icons.shield_outlined, size: 22, color: theme.colorScheme.primary),
            const SizedBox(width: 8),
            const Text('Sailor', style: TextStyle(fontWeight: FontWeight.bold, fontSize: 18)),
            const SizedBox(width: 16),
            Container(
              padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 4),
              decoration: BoxDecoration(
                color: theme.colorScheme.surfaceContainerHighest.withValues(alpha: 0.6),
                borderRadius: BorderRadius.circular(16),
                border: Border.all(color: theme.dividerColor.withValues(alpha: 0.5)),
              ),
              child: Row(
                mainAxisSize: MainAxisSize.min,
                children: [
                  Container(
                    width: 8,
                    height: 8,
                    decoration: BoxDecoration(
                      color: conn.isConnected ? Colors.greenAccent.shade400 : Colors.redAccent,
                      shape: BoxShape.circle,
                    ),
                  ),
                  const SizedBox(width: 6),
                  Text(
                    conn.isConnected ? '${conn.username}@${conn.host}:${conn.port}' : 'Disconnected',
                    style: const TextStyle(fontSize: 12, fontWeight: FontWeight.w500),
                  ),
                ],
              ),
            ),
          ],
        ),
        actions: [
          IconButton(
            icon: const Icon(Icons.settings_outlined),
            tooltip: 'Settings',
            onPressed: () {
              showDialog(
                context: context,
                builder: (_) => const SettingsDialog(),
              );
            },
          ),
          IconButton(
            icon: const Icon(Icons.logout),
            tooltip: 'Disconnect',
            onPressed: () => _handleDisconnect(context, ref),
          ),
          const SizedBox(width: 8),
        ],
      ),
      body: const Column(
        children: [
          // Dual Pane Files Explorer
          Expanded(
            flex: 6,
            child: Row(
              crossAxisAlignment: CrossAxisAlignment.stretch,
              children: [
                Expanded(child: LocalExplorerPane()),
                VerticalDivider(width: 1),
                Expanded(child: RemoteExplorerPane()),
              ],
            ),
          ),
          // Transfer Queue Panel Docked at Bottom
          SizedBox(
            height: 210,
            child: TransferQueuePanel(),
          ),
        ],
      ),
    );
  }
}

// Icon selector utility based on file extension
IconData _getFileIcon(FileEntry entry) {
  if (entry.isDirectory) return Icons.folder;
  final ext = p.extension(entry.name).toLowerCase();
  switch (ext) {
    case '.zip':
    case '.tar':
    case '.gz':
    case '.rar':
    case '.7z':
    case '.bz2':
      return Icons.archive_outlined;
    case '.png':
    case '.jpg':
    case '.jpeg':
    case '.gif':
    case '.svg':
    case '.webp':
      return Icons.image_outlined;
    case '.mp4':
    case '.mkv':
    case '.mov':
    case '.avi':
      return Icons.movie_outlined;
    case '.mp3':
    case '.wav':
    case '.flac':
    case '.ogg':
      return Icons.audio_file_outlined;
    case '.pdf':
    case '.doc':
    case '.docx':
    case '.txt':
    case '.md':
      return Icons.description_outlined;
    case '.c':
    case '.cpp':
    case '.h':
    case '.hpp':
    case '.dart':
    case '.py':
    case '.js':
    case '.ts':
    case '.json':
    case '.yaml':
    case '.yml':
    case '.html':
    case '.css':
      return Icons.code_outlined;
    default:
      return Icons.insert_drive_file_outlined;
  }
}

Color _getFileIconColor(FileEntry entry, ThemeData theme) {
  if (entry.isDirectory) return Colors.amber.shade400;
  final ext = p.extension(entry.name).toLowerCase();
  switch (ext) {
    case '.zip':
    case '.tar':
    case '.gz':
      return Colors.orangeAccent;
    case '.png':
    case '.jpg':
    case '.jpeg':
      return Colors.purpleAccent;
    case '.mp4':
    case '.mkv':
      return Colors.pinkAccent;
    case '.pdf':
      return Colors.redAccent;
    case '.c':
    case '.cpp':
    case '.dart':
    case '.py':
      return Colors.cyanAccent;
    default:
      return theme.colorScheme.onSurface.withValues(alpha: 0.6);
  }
}

// Local Explorer Pane
class LocalExplorerPane extends ConsumerWidget {
  const LocalExplorerPane({super.key});

  Future<void> _pickAndUploadFiles(BuildContext context, WidgetRef ref) async {
    final result = await FilePicker.platform.pickFiles(allowMultiple: true);
    if (result != null && result.files.isNotEmpty) {
      final remoteDir = ref.read(remoteBrowserProvider).currentPath;
      final queueNotifier = ref.read(transferQueueProvider.notifier);
      for (final f in result.files) {
        if (f.path != null) {
          queueNotifier.enqueueUpload(f.path!, remoteDir);
        }
      }
      if (context.mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text('Enqueued ${result.files.length} file(s) for upload'),
            behavior: SnackBarBehavior.floating,
          ),
        );
      }
    }
  }

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final state = ref.watch(localBrowserProvider);
    final notifier = ref.read(localBrowserProvider.notifier);
    final theme = Theme.of(context);

    return Container(
      color: theme.colorScheme.surface,
      child: Column(
        children: [
          // Local Path & Toolbar
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
            decoration: BoxDecoration(
              color: theme.colorScheme.surfaceContainerHighest.withValues(alpha: 0.25),
              border: Border(bottom: BorderSide(color: theme.dividerColor.withValues(alpha: 0.5))),
            ),
            child: Row(
              children: [
                const Icon(Icons.laptop, size: 18),
                const SizedBox(width: 8),
                const Text('Local', style: TextStyle(fontWeight: FontWeight.bold, fontSize: 13)),
                const SizedBox(width: 12),
                IconButton(
                  icon: const Icon(Icons.arrow_upward, size: 18),
                  tooltip: 'Go to parent directory',
                  onPressed: () => notifier.navigateUp(),
                  splashRadius: 18,
                ),
                IconButton(
                  icon: const Icon(Icons.refresh, size: 18),
                  tooltip: 'Refresh',
                  onPressed: () => notifier.refresh(),
                  splashRadius: 18,
                ),
                IconButton(
                  icon: const Icon(Icons.file_upload_outlined, size: 18),
                  tooltip: 'Upload custom files...',
                  onPressed: () => _pickAndUploadFiles(context, ref),
                  splashRadius: 18,
                ),
                const SizedBox(width: 8),
                Expanded(
                  child: Container(
                    padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 4),
                    decoration: BoxDecoration(
                      color: theme.colorScheme.surface,
                      borderRadius: BorderRadius.circular(6),
                      border: Border.all(color: theme.dividerColor.withValues(alpha: 0.6)),
                    ),
                    child: Text(
                      state.currentPath,
                      style: const TextStyle(fontFamily: 'monospace', fontSize: 12),
                      overflow: TextOverflow.ellipsis,
                    ),
                  ),
                ),
              ],
            ),
          ),
          // Column Headers
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 6),
            decoration: BoxDecoration(
              color: theme.colorScheme.surfaceContainerHighest.withValues(alpha: 0.1),
              border: Border(bottom: BorderSide(color: theme.dividerColor.withValues(alpha: 0.3))),
            ),
            child: const Row(
              children: [
                SizedBox(width: 28),
                Expanded(flex: 5, child: Text('Name', style: TextStyle(fontSize: 12, fontWeight: FontWeight.bold))),
                Expanded(flex: 2, child: Text('Size', style: TextStyle(fontSize: 12, fontWeight: FontWeight.bold), textAlign: TextAlign.right)),
                SizedBox(width: 16),
                Expanded(flex: 3, child: Text('Modified', style: TextStyle(fontSize: 12, fontWeight: FontWeight.bold), textAlign: TextAlign.right)),
              ],
            ),
          ),
          // File List
          Expanded(
            child: state.entries.isEmpty
                ? Center(
                    child: Text('Empty folder', style: TextStyle(color: theme.disabledColor)),
                  )
                : ListView.builder(
                    itemCount: state.entries.length,
                    itemBuilder: (context, index) {
                      final item = state.entries[index];
                      return InkWell(
                        onDoubleTap: () {
                          if (item.isDirectory) {
                            notifier.loadPath(p.join(state.currentPath, item.name));
                          }
                        },
                        onSecondaryTapDown: (details) {
                          _showLocalContextMenu(context, details.globalPosition, item, ref, state.currentPath);
                        },
                        child: Container(
                          padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 7),
                          decoration: BoxDecoration(
                            border: Border(bottom: BorderSide(color: theme.dividerColor.withValues(alpha: 0.15))),
                          ),
                          child: Row(
                            children: [
                              Icon(_getFileIcon(item), size: 18, color: _getFileIconColor(item, theme)),
                              const SizedBox(width: 10),
                              Expanded(
                                flex: 5,
                                child: Text(
                                  item.name,
                                  style: TextStyle(
                                    fontSize: 13,
                                    fontWeight: item.isDirectory ? FontWeight.w600 : FontWeight.normal,
                                  ),
                                  overflow: TextOverflow.ellipsis,
                                ),
                              ),
                              Expanded(
                                flex: 2,
                                child: Text(
                                  item.formattedSize,
                                  style: TextStyle(fontSize: 12, color: theme.colorScheme.onSurface.withValues(alpha: 0.65)),
                                  textAlign: TextAlign.right,
                                ),
                              ),
                              const SizedBox(width: 16),
                              Expanded(
                                flex: 3,
                                child: Text(
                                  item.formattedDate,
                                  style: TextStyle(fontSize: 12, color: theme.colorScheme.onSurface.withValues(alpha: 0.65)),
                                  textAlign: TextAlign.right,
                                ),
                              ),
                            ],
                          ),
                        ),
                      );
                    },
                  ),
          ),
        ],
      ),
    );
  }

  void _showLocalContextMenu(
    BuildContext context,
    Offset position,
    FileEntry entry,
    WidgetRef ref,
    String localPath,
  ) {
    final remoteState = ref.read(remoteBrowserProvider);

    showMenu(
      context: context,
      position: RelativeRect.fromLTRB(position.dx, position.dy, position.dx + 1, position.dy + 1),
      items: [
        if (!entry.isDirectory)
          PopupMenuItem(
            value: 'upload',
            child: Row(
              mainAxisSize: MainAxisSize.min,
              children: [
                const Icon(Icons.upload, size: 18, color: Colors.blueAccent),
                const SizedBox(width: 8),
                Expanded(
                  child: Text(
                    'Upload to ${remoteState.currentPath}',
                    overflow: TextOverflow.ellipsis,
                  ),
                ),
              ],
            ),
          ),
        if (entry.isDirectory)
          const PopupMenuItem(
            value: 'open',
            child: Row(
              mainAxisSize: MainAxisSize.min,
              children: [
                Icon(Icons.folder_open, size: 18),
                SizedBox(width: 8),
                Expanded(
                  child: Text('Open Folder', overflow: TextOverflow.ellipsis),
                ),
              ],
            ),
          ),
      ],
    ).then((action) {
      if (action == 'upload') {
        final filePath = p.join(localPath, entry.name);
        ref.read(transferQueueProvider.notifier).enqueueUpload(filePath, remoteState.currentPath);
        if (context.mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(
              content: Text('Enqueued "${entry.name}" for upload'),
              behavior: SnackBarBehavior.floating,
            ),
          );
        }
      } else if (action == 'open') {
        ref.read(localBrowserProvider.notifier).loadPath(p.join(localPath, entry.name));
      }
    });
  }
}

// Remote Explorer Pane
class RemoteExplorerPane extends ConsumerWidget {
  const RemoteExplorerPane({super.key});

  void _showCreateFolderDialog(BuildContext context, WidgetRef ref) {
    final controller = TextEditingController();
    showDialog(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Row(
          children: [
            Icon(Icons.create_new_folder, color: Colors.amber),
            SizedBox(width: 8),
            Text('Create New Folder'),
          ],
        ),
        content: TextField(
          controller: controller,
          autofocus: true,
          decoration: const InputDecoration(
            labelText: 'Folder Name',
            hintText: 'new_folder',
            border: OutlineInputBorder(),
          ),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.of(ctx).pop(),
            child: const Text('Cancel'),
          ),
          FilledButton(
            onPressed: () async {
              final name = controller.text.trim();
              if (name.isNotEmpty) {
                Navigator.of(ctx).pop();
                try {
                  await ref.read(remoteBrowserProvider.notifier).mkdir(name);
                } catch (e) {
                  if (context.mounted) {
                    ScaffoldMessenger.of(context).showSnackBar(
                      SnackBar(content: Text('Failed to create folder: $e'), backgroundColor: Colors.red.shade800),
                    );
                  }
                }
              }
            },
            child: const Text('Create'),
          ),
        ],
      ),
    );
  }

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final state = ref.watch(remoteBrowserProvider);
    final notifier = ref.read(remoteBrowserProvider.notifier);
    final theme = Theme.of(context);

    return Container(
      color: theme.colorScheme.surface,
      child: Column(
        children: [
          // Remote Path & Toolbar
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
            decoration: BoxDecoration(
              color: theme.colorScheme.surfaceContainerHighest.withValues(alpha: 0.25),
              border: Border(bottom: BorderSide(color: theme.dividerColor.withValues(alpha: 0.5))),
            ),
            child: Row(
              children: [
                const Icon(Icons.cloud_outlined, size: 18),
                const SizedBox(width: 8),
                const Text('Remote', style: TextStyle(fontWeight: FontWeight.bold, fontSize: 13)),
                const SizedBox(width: 12),
                IconButton(
                  icon: const Icon(Icons.arrow_upward, size: 18),
                  tooltip: 'Go to parent directory',
                  onPressed: () => notifier.navigateUp(),
                  splashRadius: 18,
                ),
                IconButton(
                  icon: const Icon(Icons.refresh, size: 18),
                  tooltip: 'Refresh',
                  onPressed: () => notifier.refresh(),
                  splashRadius: 18,
                ),
                IconButton(
                  icon: const Icon(Icons.create_new_folder_outlined, size: 18),
                  tooltip: 'New Folder',
                  onPressed: () => _showCreateFolderDialog(context, ref),
                  splashRadius: 18,
                ),
                const SizedBox(width: 8),
                Expanded(
                  child: Container(
                    padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 4),
                    decoration: BoxDecoration(
                      color: theme.colorScheme.surface,
                      borderRadius: BorderRadius.circular(6),
                      border: Border.all(color: theme.dividerColor.withValues(alpha: 0.6)),
                    ),
                    child: Text(
                      state.currentPath,
                      style: const TextStyle(fontFamily: 'monospace', fontSize: 12),
                      overflow: TextOverflow.ellipsis,
                    ),
                  ),
                ),
              ],
            ),
          ),
          // Column Headers
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 6),
            decoration: BoxDecoration(
              color: theme.colorScheme.surfaceContainerHighest.withValues(alpha: 0.1),
              border: Border(bottom: BorderSide(color: theme.dividerColor.withValues(alpha: 0.3))),
            ),
            child: const Row(
              children: [
                SizedBox(width: 28),
                Expanded(flex: 5, child: Text('Name', style: TextStyle(fontSize: 12, fontWeight: FontWeight.bold))),
                Expanded(flex: 2, child: Text('Size', style: TextStyle(fontSize: 12, fontWeight: FontWeight.bold), textAlign: TextAlign.right)),
                SizedBox(width: 16),
                Expanded(flex: 3, child: Text('Modified', style: TextStyle(fontSize: 12, fontWeight: FontWeight.bold), textAlign: TextAlign.right)),
              ],
            ),
          ),
          // File List
          Expanded(
            child: state.isLoading
                ? const Center(child: CircularProgressIndicator())
                : state.error != null
                    ? Center(
                        child: Column(
                          mainAxisSize: MainAxisSize.min,
                          children: [
                            const Icon(Icons.error_outline, color: Colors.redAccent, size: 32),
                            const SizedBox(height: 8),
                            Text(state.error!, style: const TextStyle(color: Colors.redAccent)),
                            const SizedBox(height: 12),
                            FilledButton.tonal(
                              onPressed: () => notifier.refresh(),
                              child: const Text('Retry'),
                            ),
                          ],
                        ),
                      )
                    : state.entries.isEmpty
                        ? Center(
                            child: Text('Empty directory', style: TextStyle(color: theme.disabledColor)),
                          )
                        : ListView.builder(
                            itemCount: state.entries.length,
                            itemBuilder: (context, index) {
                              final item = state.entries[index];
                              return InkWell(
                                onDoubleTap: () {
                                  if (item.isDirectory) {
                                    final next = state.currentPath == '/'
                                        ? '/${item.name}'
                                        : '${state.currentPath}/${item.name}';
                                    notifier.loadPath(next);
                                  }
                                },
                                onSecondaryTapDown: (details) {
                                  _showRemoteContextMenu(context, details.globalPosition, item, ref);
                                },
                                child: Container(
                                  padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 7),
                                  decoration: BoxDecoration(
                                    border: Border(bottom: BorderSide(color: theme.dividerColor.withValues(alpha: 0.15))),
                                  ),
                                  child: Row(
                                    children: [
                                      Icon(_getFileIcon(item), size: 18, color: _getFileIconColor(item, theme)),
                                      const SizedBox(width: 10),
                                      Expanded(
                                        flex: 5,
                                        child: Text(
                                          item.name,
                                          style: TextStyle(
                                            fontSize: 13,
                                            fontWeight: item.isDirectory ? FontWeight.w600 : FontWeight.normal,
                                          ),
                                          overflow: TextOverflow.ellipsis,
                                        ),
                                      ),
                                      Expanded(
                                        flex: 2,
                                        child: Text(
                                          item.formattedSize,
                                          style: TextStyle(fontSize: 12, color: theme.colorScheme.onSurface.withValues(alpha: 0.65)),
                                          textAlign: TextAlign.right,
                                        ),
                                      ),
                                      const SizedBox(width: 16),
                                      Expanded(
                                        flex: 3,
                                        child: Text(
                                          item.formattedDate,
                                          style: TextStyle(fontSize: 12, color: theme.colorScheme.onSurface.withValues(alpha: 0.65)),
                                          textAlign: TextAlign.right,
                                        ),
                                      ),
                                    ],
                                  ),
                                ),
                              );
                            },
                          ),
          ),
        ],
      ),
    );
  }

  void _showRemoteContextMenu(
    BuildContext context,
    Offset position,
    FileEntry entry,
    WidgetRef ref,
  ) {
    final remoteState = ref.read(remoteBrowserProvider);
    final localState = ref.read(localBrowserProvider);
    final fullRemotePath = remoteState.currentPath == '/' ? '/${entry.name}' : '${remoteState.currentPath}/${entry.name}';

    showMenu(
      context: context,
      position: RelativeRect.fromLTRB(position.dx, position.dy, position.dx + 1, position.dy + 1),
      items: [
        if (!entry.isDirectory) ...[
          PopupMenuItem(
            value: 'download_current',
            child: Row(
              mainAxisSize: MainAxisSize.min,
              children: [
                const Icon(Icons.download, size: 18, color: Colors.tealAccent),
                const SizedBox(width: 8),
                Expanded(
                  child: Text(
                    'Download to ${localState.currentPath}',
                    overflow: TextOverflow.ellipsis,
                  ),
                ),
              ],
            ),
          ),
          const PopupMenuItem(
            value: 'download_custom',
            child: Row(
              mainAxisSize: MainAxisSize.min,
              children: [
                Icon(Icons.file_download_outlined, size: 18),
                SizedBox(width: 8),
                Expanded(
                  child: Text('Download To...', overflow: TextOverflow.ellipsis),
                ),
              ],
            ),
          ),
        ],
        const PopupMenuItem(
          value: 'rename',
          child: Row(
            mainAxisSize: MainAxisSize.min,
            children: [
              Icon(Icons.drive_file_rename_outline, size: 18),
              SizedBox(width: 8),
              Expanded(
                child: Text('Rename', overflow: TextOverflow.ellipsis),
              ),
            ],
          ),
        ),
        PopupMenuItem(
          value: 'delete',
          child: Row(
            mainAxisSize: MainAxisSize.min,
            children: [
              const Icon(Icons.delete_outline, size: 18, color: Colors.redAccent),
              const SizedBox(width: 8),
              Expanded(
                child: Text(
                  entry.isDirectory ? 'Delete Folder' : 'Delete File',
                  style: const TextStyle(color: Colors.redAccent),
                  overflow: TextOverflow.ellipsis,
                ),
              ),
            ],
          ),
        ),
      ],
    ).then((action) async {
      if (action == 'download_current') {
        ref.read(transferQueueProvider.notifier).enqueueDownload(
          fullRemotePath,
          entry.name,
          entry.size,
          localState.currentPath,
        );
        if (context.mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(
              content: Text('Enqueued "${entry.name}" for download'),
              behavior: SnackBarBehavior.floating,
            ),
          );
        }
      } else if (action == 'download_custom') {
        final targetDir = await FilePicker.platform.getDirectoryPath(dialogTitle: 'Select Download Destination');
        if (targetDir != null) {
          ref.read(transferQueueProvider.notifier).enqueueDownload(
            fullRemotePath,
            entry.name,
            entry.size,
            targetDir,
          );
          if (context.mounted) {
            ScaffoldMessenger.of(context).showSnackBar(
              SnackBar(
                content: Text('Enqueued "${entry.name}" for download to $targetDir'),
                behavior: SnackBarBehavior.floating,
              ),
            );
          }
        }
      } else if (action == 'rename') {
        if (!context.mounted) return;
        final controller = TextEditingController(text: entry.name);
        showDialog(
          context: context,
          builder: (ctx) => AlertDialog(
            title: const Row(
              children: [
                Icon(Icons.drive_file_rename_outline),
                SizedBox(width: 8),
                Text('Rename Entry'),
              ],
            ),
            content: TextField(
              controller: controller,
              autofocus: true,
              decoration: const InputDecoration(labelText: 'New Name', border: OutlineInputBorder()),
            ),
            actions: [
              TextButton(onPressed: () => Navigator.of(ctx).pop(), child: const Text('Cancel')),
              FilledButton(
                onPressed: () async {
                  final newName = controller.text.trim();
                  if (newName.isNotEmpty && newName != entry.name) {
                    Navigator.of(ctx).pop();
                    try {
                      await ref.read(remoteBrowserProvider.notifier).rename(entry.name, newName);
                    } catch (e) {
                      if (context.mounted) {
                        ScaffoldMessenger.of(context).showSnackBar(
                          SnackBar(content: Text('Rename failed: $e'), backgroundColor: Colors.red.shade800),
                        );
                      }
                    }
                  }
                },
                child: const Text('Rename'),
              ),
            ],
          ),
        );
      } else if (action == 'delete') {
        if (!context.mounted) return;
        showDialog(
          context: context,
          builder: (ctx) => AlertDialog(
            title: const Row(
              children: [
                Icon(Icons.warning_amber_rounded, color: Colors.redAccent),
                SizedBox(width: 8),
                Text('Confirm Delete'),
              ],
            ),
            content: Text('Are you sure you want to permanently delete "${entry.name}"?'),
            actions: [
              TextButton(onPressed: () => Navigator.of(ctx).pop(), child: const Text('Cancel')),
              FilledButton(
                style: FilledButton.styleFrom(backgroundColor: Colors.redAccent),
                onPressed: () async {
                  Navigator.of(ctx).pop();
                  try {
                    await ref.read(remoteBrowserProvider.notifier).deletePath(entry);
                  } catch (e) {
                    if (context.mounted) {
                      ScaffoldMessenger.of(context).showSnackBar(
                        SnackBar(content: Text('Delete failed: $e'), backgroundColor: Colors.red.shade800),
                      );
                    }
                  }
                },
                child: const Text('Delete'),
              ),
            ],
          ),
        );
      }
    });
  }
}
