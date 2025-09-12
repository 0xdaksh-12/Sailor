import 'package:flutter_test/flutter_test.dart';
import 'package:sailor_desktop/core/models/models.dart';

void main() {
  group('FileEntry tests', () {
    test('FileEntry formats sizes properly', () {
      final dir = FileEntry(
        name: 'docs',
        isDirectory: true,
        size: 0,
        modifiedTime: DateTime(2026, 9, 16, 12, 0),
      );
      expect(dir.formattedSize, '--');

      final small = FileEntry(
        name: 'test.txt',
        isDirectory: false,
        size: 512,
        modifiedTime: DateTime(2026, 9, 16, 12, 0),
      );
      expect(small.formattedSize, '512 B');

      final medium = FileEntry(
        name: 'image.png',
        isDirectory: false,
        size: 2048 * 1024,
        modifiedTime: DateTime(2026, 9, 16, 12, 0),
      );
      expect(medium.formattedSize, '2.0 MB');

      final large = FileEntry(
        name: 'video.mp4',
        isDirectory: false,
        size: 3 * 1024 * 1024 * 1024,
        modifiedTime: DateTime(2026, 9, 16, 12, 0),
      );
      expect(large.formattedSize, '3.00 GB');
    });

    test('FileEntry formats date properly', () {
      final entry = FileEntry(
        name: 'report.pdf',
        isDirectory: false,
        size: 1024,
        modifiedTime: DateTime(2026, 9, 16, 14, 30),
      );
      expect(entry.formattedDate, '2026-09-16 14:30');
    });
  });

  group('TransferTask tests', () {
    test('TransferTask calculates progress accurately', () {
      final task = TransferTask(
        id: '1',
        filename: 'archive.tar.gz',
        localPath: '/local/archive.tar.gz',
        remotePath: '/remote/archive.tar.gz',
        totalBytes: 1000,
        direction: TransferDirection.upload,
        transferredBytes: 250,
      );

      expect(task.progress, 0.25);
      expect(task.formattedProgress, '25.0%');

      task.transferredBytes = 1000;
      expect(task.progress, 1.0);
      expect(task.formattedProgress, '100.0%');
    });
  });

  group('ConnectionStateData tests', () {
    test('Default connection state is disconnected', () {
      const state = ConnectionStateData();
      expect(state.isConnected, isFalse);
      expect(state.host, '');
      expect(state.port, 9000);
    });

    test('copyWith updates fields', () {
      const state = ConnectionStateData();
      final updated = state.copyWith(
        isConnected: true,
        host: '127.0.0.1',
        username: 'admin',
      );
      expect(updated.isConnected, isTrue);
      expect(updated.host, '127.0.0.1');
      expect(updated.username, 'admin');
    });
  });
}
