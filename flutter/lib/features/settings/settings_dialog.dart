import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:file_picker/file_picker.dart';
import '../providers.dart';

class SettingsDialog extends ConsumerStatefulWidget {
  const SettingsDialog({super.key});

  @override
  ConsumerState<SettingsDialog> createState() => _SettingsDialogState();
}

class _SettingsDialogState extends ConsumerState<SettingsDialog> {
  late TextEditingController _downloadDirController;
  late TextEditingController _hostController;
  late TextEditingController _portController;
  late TextEditingController _userController;
  late ThemeMode _selectedTheme;

  @override
  void initState() {
    super.initState();
    final settings = ref.read(settingsProvider);
    _downloadDirController = TextEditingController(text: settings.defaultDownloadDir);
    _hostController = TextEditingController(text: settings.defaultHost);
    _portController = TextEditingController(text: settings.defaultPort.toString());
    _userController = TextEditingController(text: settings.defaultUsername);
    _selectedTheme = settings.themeMode;
  }

  @override
  void dispose() {
    _downloadDirController.dispose();
    _hostController.dispose();
    _portController.dispose();
    _userController.dispose();
    super.dispose();
  }

  Future<void> _pickDownloadDir() async {
    final dir = await FilePicker.platform.getDirectoryPath(
      dialogTitle: 'Select Default Download Directory',
      initialDirectory: _downloadDirController.text.isNotEmpty ? _downloadDirController.text : null,
    );
    if (dir != null) {
      setState(() {
        _downloadDirController.text = dir;
      });
    }
  }

  Future<void> _saveSettings() async {
    final notifier = ref.read(settingsProvider.notifier);
    await notifier.updateTheme(_selectedTheme);
    await notifier.updateDefaults(
      downloadDir: _downloadDirController.text.trim(),
      host: _hostController.text.trim(),
      port: int.tryParse(_portController.text.trim()) ?? 9000,
      username: _userController.text.trim(),
    );
    if (mounted) {
      Navigator.of(context).pop();
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text('Settings saved successfully'),
          behavior: SnackBarBehavior.floating,
        ),
      );
    }
  }

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);

    return Dialog(
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
      child: Container(
        width: 520,
        padding: const EdgeInsets.all(24),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            Row(
              children: [
                Icon(Icons.settings, color: theme.colorScheme.primary, size: 24),
                const SizedBox(width: 10),
                Text('Application Settings', style: theme.textTheme.titleLarge?.copyWith(fontWeight: FontWeight.bold)),
                const Spacer(),
                IconButton(
                  icon: const Icon(Icons.close),
                  onPressed: () => Navigator.of(context).pop(),
                  splashRadius: 20,
                ),
              ],
            ),
            const Divider(height: 24),
            // Theme selector
            Row(
              children: [
                const Icon(Icons.palette_outlined, size: 20),
                const SizedBox(width: 12),
                const Text('Theme Mode:', style: TextStyle(fontWeight: FontWeight.w500)),
                const Spacer(),
                SegmentedButton<ThemeMode>(
                  segments: const [
                    ButtonSegment(value: ThemeMode.dark, label: Text('Dark'), icon: Icon(Icons.dark_mode_outlined, size: 16)),
                    ButtonSegment(value: ThemeMode.light, label: Text('Light'), icon: Icon(Icons.light_mode_outlined, size: 16)),
                    ButtonSegment(value: ThemeMode.system, label: Text('System'), icon: Icon(Icons.computer_outlined, size: 16)),
                  ],
                  selected: {_selectedTheme},
                  onSelectionChanged: (set) {
                    setState(() {
                      _selectedTheme = set.first;
                    });
                  },
                ),
              ],
            ),
            const SizedBox(height: 16),
            // Default Download Directory
            Row(
              crossAxisAlignment: CrossAxisAlignment.center,
              children: [
                Expanded(
                  child: TextField(
                    controller: _downloadDirController,
                    decoration: const InputDecoration(
                      labelText: 'Default Download Directory',
                      prefixIcon: Icon(Icons.folder_outlined, size: 20),
                      border: OutlineInputBorder(),
                      isDense: true,
                    ),
                  ),
                ),
                const SizedBox(width: 8),
                IconButton.filledTonal(
                  icon: const Icon(Icons.folder_open),
                  tooltip: 'Browse Folder',
                  onPressed: _pickDownloadDir,
                ),
              ],
            ),
            const SizedBox(height: 16),
            // Default Connection Details
            Row(
              children: [
                Expanded(
                  flex: 3,
                  child: TextField(
                    controller: _hostController,
                    decoration: const InputDecoration(
                      labelText: 'Default Host',
                      prefixIcon: Icon(Icons.dns_outlined, size: 20),
                      border: OutlineInputBorder(),
                      isDense: true,
                    ),
                  ),
                ),
                const SizedBox(width: 12),
                Expanded(
                  flex: 2,
                  child: TextField(
                    controller: _portController,
                    decoration: const InputDecoration(
                      labelText: 'Default Port',
                      prefixIcon: Icon(Icons.tag, size: 20),
                      border: OutlineInputBorder(),
                      isDense: true,
                    ),
                  ),
                ),
              ],
            ),
            const SizedBox(height: 16),
            TextField(
              controller: _userController,
              decoration: const InputDecoration(
                labelText: 'Default Username',
                prefixIcon: Icon(Icons.person_outline, size: 20),
                border: OutlineInputBorder(),
                isDense: true,
              ),
            ),
            const SizedBox(height: 24),
            Row(
              mainAxisAlignment: MainAxisAlignment.end,
              children: [
                TextButton(
                  onPressed: () => Navigator.of(context).pop(),
                  child: const Text('Cancel'),
                ),
                const SizedBox(width: 12),
                FilledButton.icon(
                  onPressed: _saveSettings,
                  icon: const Icon(Icons.save_outlined, size: 18),
                  label: const Text('Save Changes'),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }
}
