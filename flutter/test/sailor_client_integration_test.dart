import 'dart:io';
import 'package:flutter_test/flutter_test.dart';
import 'package:sailor_desktop/core/sailor_client.dart';

void main() {
  group('SailorClient FFI Integration Tests', () {
    late SailorClient client;
    Process? serverProcess;
    final workspaceDir = Directory.current.path.endsWith('flutter')
        ? Directory.current.parent.path
        : Directory.current.path;

    setUpAll(() async {
      // Start the Sailor Server in background
      final serverExe = '$workspaceDir/build/sailor-server';
      final cert = '$workspaceDir/certs/cert.pem';
      final key = '$workspaceDir/certs/key.pem';
      final db = '$workspaceDir/data/users.db';
      final storage = '$workspaceDir/server_storage';

      if (File(serverExe).existsSync()) {
        serverProcess = await Process.start(serverExe, [cert, key, '9001', db, storage]);
        await Future.delayed(const Duration(milliseconds: 500));
      }

      client = SailorClient(
        customLibraryPath: '$workspaceDir/build/libsailor.so',
      );
      await client.init();
    });

    tearDownAll(() async {
      client.dispose();
      serverProcess?.kill();
    });

    test('Full Client Lifecycle: Connect, List, Mkdir, Upload, Download, Rename, Delete, Rmdir, Disconnect', () async {
      if (serverProcess == null) {
        // Skip if server binary not built yet
        return;
      }

      // Connect
      await client.connect('127.0.0.1', 9001, 'admin', 'password123');
      final connected = await client.isConnected();
      expect(connected, isTrue);

      // List Root
      final rootList = await client.list('/');
      expect(rootList, isNotNull);

      // Mkdir
      await client.mkdir('/flutter_test_dir');
      final listAfterMkdir = await client.list('/');
      expect(listAfterMkdir.any((e) => e.name == 'flutter_test_dir' && e.isDirectory), isTrue);

      // Create a local test file
      final tempFile = File('$workspaceDir/flutter_test_sample.txt');
      await tempFile.writeAsString('Hello Sailor Desktop from Flutter!');

      // Upload
      await client.upload(tempFile.path, '/flutter_test_dir');
      final dirListing = await client.list('/flutter_test_dir');
      expect(dirListing.any((e) => e.name == 'flutter_test_sample.txt'), isTrue);

      // Download
      final downloadDest = File('$workspaceDir/flutter_downloaded_sample.txt');
      if (downloadDest.existsSync()) downloadDest.deleteSync();
      await client.download('/flutter_test_dir/flutter_test_sample.txt', downloadDest.path);
      expect(downloadDest.existsSync(), isTrue);
      expect(await downloadDest.readAsString(), 'Hello Sailor Desktop from Flutter!');

      // Rename
      await client.rename(
        '/flutter_test_dir/flutter_test_sample.txt',
        '/flutter_test_dir/renamed_sample.txt',
      );
      final listAfterRename = await client.list('/flutter_test_dir');
      expect(listAfterRename.any((e) => e.name == 'renamed_sample.txt'), isTrue);
      expect(listAfterRename.any((e) => e.name == 'flutter_test_sample.txt'), isFalse);

      // Delete file
      await client.delete('/flutter_test_dir/renamed_sample.txt');
      final listAfterDelete = await client.list('/flutter_test_dir');
      expect(listAfterDelete.any((e) => e.name == 'renamed_sample.txt'), isFalse);

      // Rmdir
      await client.rmdir('/flutter_test_dir');
      final listAfterRmdir = await client.list('/');
      expect(listAfterRmdir.any((e) => e.name == 'flutter_test_dir'), isFalse);

      // Disconnect
      await client.disconnect();

      // Clean local test artifacts
      if (tempFile.existsSync()) tempFile.deleteSync();
      if (downloadDest.existsSync()) downloadDest.deleteSync();
    });
  });
}
