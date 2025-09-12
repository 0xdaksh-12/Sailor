import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../../core/models/models.dart';
import '../providers.dart';

class TransferQueuePanel extends ConsumerWidget {
  const TransferQueuePanel({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final tasks = ref.watch(transferQueueProvider);
    final queueNotifier = ref.read(transferQueueProvider.notifier);
    final theme = Theme.of(context);

    final activeTasks = tasks.where((t) => t.status == TransferStatus.inProgress || t.status == TransferStatus.queued).toList();
    final completedTasks = tasks.where((t) => t.status == TransferStatus.completed).toList();
    final failedTasks = tasks.where((t) => t.status == TransferStatus.failed).toList();

    return Container(
      decoration: BoxDecoration(
        color: theme.colorScheme.surface,
        border: Border(
          top: BorderSide(color: theme.dividerColor, width: 1),
        ),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          // Header Bar
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
            decoration: BoxDecoration(
              color: theme.colorScheme.surfaceContainerHighest.withValues(alpha: 0.4),
              border: Border(
                bottom: BorderSide(color: theme.dividerColor.withValues(alpha: 0.5), width: 1),
              ),
            ),
            child: Row(
              children: [
                Icon(Icons.swap_vert, size: 18, color: theme.colorScheme.primary),
                const SizedBox(width: 8),
                Text(
                  'Transfer Queue',
                  style: theme.textTheme.titleSmall?.copyWith(fontWeight: FontWeight.bold),
                ),
                const SizedBox(width: 12),
                if (activeTasks.isNotEmpty)
                  Container(
                    padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 2),
                    decoration: BoxDecoration(
                      color: theme.colorScheme.primaryContainer,
                      borderRadius: BorderRadius.circular(12),
                    ),
                    child: Text(
                      '${activeTasks.length} active',
                      style: TextStyle(
                        fontSize: 11,
                        fontWeight: FontWeight.bold,
                        color: theme.colorScheme.onPrimaryContainer,
                      ),
                    ),
                  ),
                if (failedTasks.isNotEmpty) ...[
                  const SizedBox(width: 6),
                  Container(
                    padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 2),
                    decoration: BoxDecoration(
                      color: theme.colorScheme.errorContainer,
                      borderRadius: BorderRadius.circular(12),
                    ),
                    child: Text(
                      '${failedTasks.length} failed',
                      style: TextStyle(
                        fontSize: 11,
                        fontWeight: FontWeight.bold,
                        color: theme.colorScheme.onErrorContainer,
                      ),
                    ),
                  ),
                ],
                const Spacer(),
                if (completedTasks.isNotEmpty || failedTasks.isNotEmpty)
                  TextButton.icon(
                    style: TextButton.styleFrom(
                      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
                      visualDensity: VisualDensity.compact,
                    ),
                    icon: const Icon(Icons.clear_all, size: 16),
                    label: const Text('Clear Inactive', style: TextStyle(fontSize: 12)),
                    onPressed: () => queueNotifier.clearCompleted(),
                  ),
              ],
            ),
          ),
          // Queue Task List
          Expanded(
            child: tasks.isEmpty
                ? Center(
                    child: Row(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        Icon(Icons.inbox_outlined, size: 20, color: theme.disabledColor),
                        const SizedBox(width: 8),
                        Text('No transfer tasks in queue', style: TextStyle(color: theme.disabledColor, fontSize: 13)),
                      ],
                    ),
                  )
                : ListView.separated(
                    itemCount: tasks.length,
                    separatorBuilder: (_, __) => Divider(height: 1, color: theme.dividerColor.withValues(alpha: 0.3)),
                    itemBuilder: (context, index) {
                      final task = tasks[index];
                      return _TransferTaskTile(task: task, notifier: queueNotifier);
                    },
                  ),
          ),
        ],
      ),
    );
  }
}

class _TransferTaskTile extends StatelessWidget {
  final TransferTask task;
  final TransferNotifier notifier;

  const _TransferTaskTile({
    required this.task,
    required this.notifier,
  });

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final isUpload = task.direction == TransferDirection.upload;

    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.center,
        children: [
          // Direction icon
          Container(
            padding: const EdgeInsets.all(6),
            decoration: BoxDecoration(
              color: isUpload ? Colors.blue.withValues(alpha: 0.12) : Colors.teal.withValues(alpha: 0.12),
              borderRadius: BorderRadius.circular(6),
            ),
            child: Icon(
              isUpload ? Icons.upload : Icons.download,
              size: 18,
              color: isUpload ? Colors.blue : Colors.teal,
            ),
          ),
          const SizedBox(width: 12),
          // File Name & Path
          Expanded(
            flex: 3,
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              mainAxisSize: MainAxisSize.min,
              children: [
                Text(
                  task.filename,
                  style: const TextStyle(fontWeight: FontWeight.w600, fontSize: 13),
                  overflow: TextOverflow.ellipsis,
                ),
                const SizedBox(height: 2),
                Text(
                  isUpload ? '-> ${task.remotePath}' : '<- ${task.remotePath}',
                  style: TextStyle(
                    fontSize: 11,
                    color: theme.colorScheme.onSurface.withValues(alpha: 0.55),
                    fontFamily: 'monospace',
                  ),
                  overflow: TextOverflow.ellipsis,
                ),
              ],
            ),
          ),
          const SizedBox(width: 16),
          // Progress bar & metrics
          Expanded(
            flex: 4,
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              mainAxisSize: MainAxisSize.min,
              children: [
                ClipRRect(
                  borderRadius: BorderRadius.circular(4),
                  child: LinearProgressIndicator(
                    value: task.status == TransferStatus.completed
                        ? 1.0
                        : (task.status == TransferStatus.queued ? null : task.progress),
                    minHeight: 6,
                    backgroundColor: theme.colorScheme.surfaceContainerHighest,
                    valueColor: AlwaysStoppedAnimation<Color>(
                      task.status == TransferStatus.failed
                          ? Colors.redAccent
                          : (task.status == TransferStatus.completed ? Colors.green : theme.colorScheme.primary),
                    ),
                  ),
                ),
                const SizedBox(height: 4),
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    Text(
                      task.formattedTransferred,
                      style: TextStyle(fontSize: 11, color: theme.colorScheme.onSurface.withValues(alpha: 0.6)),
                    ),
                    Text(
                      task.formattedProgress,
                      style: const TextStyle(fontSize: 11, fontWeight: FontWeight.bold),
                    ),
                  ],
                ),
              ],
            ),
          ),
          const SizedBox(width: 20),
          // Status Badge
          SizedBox(
            width: 110,
            child: _buildStatusBadge(theme),
          ),
          const SizedBox(width: 8),
          // Actions (Cancel or Retry)
          if (task.status == TransferStatus.failed)
            IconButton(
              icon: const Icon(Icons.replay, size: 18),
              tooltip: 'Retry transfer',
              onPressed: () => notifier.retryTask(task.id),
              splashRadius: 18,
            )
          else if (task.status == TransferStatus.queued || task.status == TransferStatus.inProgress)
            IconButton(
              icon: const Icon(Icons.close, size: 18),
              tooltip: 'Cancel transfer',
              onPressed: () => notifier.cancelTask(task.id),
              splashRadius: 18,
            )
          else
            const SizedBox(width: 36),
        ],
      ),
    );
  }

  Widget _buildStatusBadge(ThemeData theme) {
    switch (task.status) {
      case TransferStatus.queued:
        return const Row(
          children: [
            Icon(Icons.hourglass_empty, size: 14, color: Colors.amber),
            SizedBox(width: 4),
            Text('Queued', style: TextStyle(fontSize: 12, color: Colors.amber, fontWeight: FontWeight.w500)),
          ],
        );
      case TransferStatus.inProgress:
        return Row(
          children: [
            const SizedBox(
              width: 12,
              height: 12,
              child: CircularProgressIndicator(strokeWidth: 2),
            ),
            const SizedBox(width: 6),
            Text('Transferring', style: TextStyle(fontSize: 12, color: theme.colorScheme.primary, fontWeight: FontWeight.w500)),
          ],
        );
      case TransferStatus.completed:
        return const Row(
          children: [
            Icon(Icons.check_circle, size: 14, color: Colors.green),
            SizedBox(width: 4),
            Text('Completed', style: TextStyle(fontSize: 12, color: Colors.green, fontWeight: FontWeight.w500)),
          ],
        );
      case TransferStatus.failed:
        return Tooltip(
          message: task.errorMessage ?? 'Unknown error',
          child: const Row(
            children: [
              Icon(Icons.error_outline, size: 14, color: Colors.redAccent),
              SizedBox(width: 4),
              Text('Failed', style: TextStyle(fontSize: 12, color: Colors.redAccent, fontWeight: FontWeight.w500)),
            ],
          ),
        );
      case TransferStatus.cancelled:
        return Row(
          children: [
            Icon(Icons.cancel_outlined, size: 14, color: theme.disabledColor),
            const SizedBox(width: 4),
            Text('Cancelled', style: TextStyle(fontSize: 12, color: theme.disabledColor)),
          ],
        );
    }
  }
}
