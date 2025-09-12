import 'package:intl/intl.dart';

class FileEntry {
  final String name;
  final bool isDirectory;
  final int size;
  final DateTime modifiedTime;

  const FileEntry({
    required this.name,
    required this.isDirectory,
    required this.size,
    required this.modifiedTime,
  });

  String get formattedSize {
    if (isDirectory) return '--';
    if (size < 1024) return '$size B';
    if (size < 1024 * 1024) return '${(size / 1024).toStringAsFixed(1)} KB';
    if (size < 1024 * 1024 * 1024) {
      return '${(size / (1024 * 1024)).toStringAsFixed(1)} MB';
    }
    return '${(size / (1024 * 1024 * 1024)).toStringAsFixed(2)} GB';
  }

  String get formattedDate {
    if (modifiedTime.millisecondsSinceEpoch <= 0) return '--';
    final formatter = DateFormat('yyyy-MM-dd HH:mm');
    return formatter.format(modifiedTime);
  }
}

enum TransferStatus { queued, inProgress, completed, failed, cancelled }

enum TransferDirection { upload, download }

class TransferTask {
  final String id;
  final String filename;
  final String localPath;
  final String remotePath;
  final int totalBytes;
  final TransferDirection direction;
  int transferredBytes;
  TransferStatus status;
  String? errorMessage;
  DateTime startTime;
  DateTime? endTime;

  TransferTask({
    required this.id,
    required this.filename,
    required this.localPath,
    required this.remotePath,
    required this.totalBytes,
    required this.direction,
    this.transferredBytes = 0,
    this.status = TransferStatus.queued,
    this.errorMessage,
    DateTime? startTime,
    this.endTime,
  }) : startTime = startTime ?? DateTime.now();

  double get progress {
    if (totalBytes <= 0) {
      return status == TransferStatus.completed ? 1.0 : 0.0;
    }
    return (transferredBytes / totalBytes).clamp(0.0, 1.0);
  }

  String get formattedProgress {
    return '${(progress * 100).toStringAsFixed(1)}%';
  }

  String get formattedTransferred {
    String formatBytes(int b) {
      if (b < 1024) return '$b B';
      if (b < 1024 * 1024) return '${(b / 1024).toStringAsFixed(1)} KB';
      if (b < 1024 * 1024 * 1024) {
        return '${(b / (1024 * 1024)).toStringAsFixed(1)} MB';
      }
      return '${(b / (1024 * 1024 * 1024)).toStringAsFixed(2)} GB';
    }

    return '${formatBytes(transferredBytes)} / ${formatBytes(totalBytes)}';
  }
}

class ConnectionStateData {
  final bool isConnected;
  final String host;
  final int port;
  final String username;
  final String? error;
  final DateTime? connectedAt;

  const ConnectionStateData({
    this.isConnected = false,
    this.host = '',
    this.port = 9000,
    this.username = '',
    this.error,
    this.connectedAt,
  });

  ConnectionStateData copyWith({
    bool? isConnected,
    String? host,
    int? port,
    String? username,
    String? error,
    DateTime? connectedAt,
  }) {
    return ConnectionStateData(
      isConnected: isConnected ?? this.isConnected,
      host: host ?? this.host,
      port: port ?? this.port,
      username: username ?? this.username,
      error: error,
      connectedAt: connectedAt ?? this.connectedAt,
    );
  }
}
