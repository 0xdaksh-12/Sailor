import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../explorer/explorer_screen.dart';
import '../providers.dart';
import '../settings/settings_dialog.dart';

class ConnectScreen extends ConsumerStatefulWidget {
  const ConnectScreen({super.key});

  @override
  ConsumerState<ConnectScreen> createState() => _ConnectScreenState();
}

class _ConnectScreenState extends ConsumerState<ConnectScreen> {
  final _formKey = GlobalKey<FormState>();
  late TextEditingController _hostController;
  late TextEditingController _portController;
  late TextEditingController _userController;
  late TextEditingController _passController;
  bool _obscurePassword = true;
  bool _isLoading = false;

  @override
  void initState() {
    super.initState();
    final settings = ref.read(settingsProvider);
    _hostController = TextEditingController(text: settings.defaultHost);
    _portController = TextEditingController(text: settings.defaultPort.toString());
    _userController = TextEditingController(text: settings.defaultUsername);
    _passController = TextEditingController(text: 'password123');
  }

  @override
  void dispose() {
    _hostController.dispose();
    _portController.dispose();
    _userController.dispose();
    _passController.dispose();
    super.dispose();
  }

  Future<void> _handleConnect() async {
    if (!_formKey.currentState!.validate()) return;

    setState(() => _isLoading = true);
    final notifier = ref.read(connectionProvider.notifier);

    final host = _hostController.text.trim();
    final port = int.parse(_portController.text.trim());
    final user = _userController.text.trim();
    final pass = _passController.text;

    final success = await notifier.connect(
      host: host,
      port: port,
      username: user,
      password: pass,
    );

    setState(() => _isLoading = false);

    if (success && mounted) {
      Navigator.of(context).pushReplacement(
        MaterialPageRoute(builder: (_) => const ExplorerScreen()),
      );
    } else if (mounted) {
      final error = ref.read(connectionProvider).error ?? 'Authentication or connection failed';
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Row(
            children: [
              const Icon(Icons.error_outline, color: Colors.white, size: 20),
              const SizedBox(width: 8),
              Expanded(child: Text(error)),
            ],
          ),
          backgroundColor: Colors.red.shade800,
          behavior: SnackBarBehavior.floating,
        ),
      );
    }
  }

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);

    return Scaffold(
      body: Stack(
        children: [
          // Background subtle pattern
          Container(
            decoration: BoxDecoration(
              gradient: LinearGradient(
                begin: Alignment.topLeft,
                end: Alignment.bottomRight,
                colors: [
                  theme.colorScheme.surface,
                  theme.colorScheme.surfaceContainerHighest.withValues(alpha: 0.4),
                  theme.colorScheme.surface,
                ],
              ),
            ),
          ),
          // Top right settings action
          Positioned(
            top: 24,
            right: 24,
            child: IconButton.outlined(
              icon: const Icon(Icons.settings_outlined),
              tooltip: 'Settings',
              onPressed: () {
                showDialog(
                  context: context,
                  builder: (_) => const SettingsDialog(),
                );
              },
            ),
          ),
          // Main Center Card
          Center(
            child: SingleChildScrollView(
              padding: const EdgeInsets.all(24),
              child: Container(
                width: 440,
                decoration: BoxDecoration(
                  color: theme.colorScheme.surface,
                  borderRadius: BorderRadius.circular(20),
                  border: Border.all(color: theme.dividerColor.withValues(alpha: 0.6)),
                  boxShadow: [
                    BoxShadow(
                      color: Colors.black.withValues(alpha: 0.2),
                      blurRadius: 24,
                      offset: const Offset(0, 8),
                    ),
                  ],
                ),
                padding: const EdgeInsets.all(32),
                child: Form(
                  key: _formKey,
                  child: Column(
                    mainAxisSize: MainAxisSize.min,
                    crossAxisAlignment: CrossAxisAlignment.stretch,
                    children: [
                      // Header Logo & Branding
                      Center(
                        child: Container(
                          padding: const EdgeInsets.all(16),
                          decoration: BoxDecoration(
                            color: theme.colorScheme.primaryContainer.withValues(alpha: 0.6),
                            shape: BoxShape.circle,
                          ),
                          child: Icon(
                            Icons.shield_outlined,
                            size: 40,
                            color: theme.colorScheme.primary,
                          ),
                        ),
                      ),
                      const SizedBox(height: 16),
                      Text(
                        'Sailor File Transfer',
                        textAlign: TextAlign.center,
                        style: theme.textTheme.headlineSmall?.copyWith(
                          fontWeight: FontWeight.bold,
                          letterSpacing: 0.5,
                        ),
                      ),
                      const SizedBox(height: 6),
                      Text(
                        'Secure TLS Client for Remote File Management',
                        textAlign: TextAlign.center,
                        style: theme.textTheme.bodySmall?.copyWith(
                          color: theme.colorScheme.onSurface.withValues(alpha: 0.6),
                        ),
                      ),
                      const SizedBox(height: 28),
                      // Host & Port Row
                      Row(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Expanded(
                            flex: 3,
                            child: TextFormField(
                              controller: _hostController,
                              decoration: const InputDecoration(
                                labelText: 'Host / IP',
                                prefixIcon: Icon(Icons.dns_outlined, size: 20),
                                border: OutlineInputBorder(),
                                hintText: '127.0.0.1',
                              ),
                              validator: (val) => (val == null || val.trim().isEmpty) ? 'Required' : null,
                            ),
                          ),
                          const SizedBox(width: 12),
                          Expanded(
                            flex: 2,
                            child: TextFormField(
                              controller: _portController,
                              keyboardType: TextInputType.number,
                              decoration: const InputDecoration(
                                labelText: 'Port',
                                prefixIcon: Icon(Icons.tag, size: 20),
                                border: OutlineInputBorder(),
                                hintText: '9000',
                              ),
                              validator: (val) {
                                if (val == null || val.trim().isEmpty) return 'Required';
                                final p = int.tryParse(val.trim());
                                if (p == null || p <= 0 || p > 65535) return 'Invalid';
                                return null;
                              },
                            ),
                          ),
                        ],
                      ),
                      const SizedBox(height: 16),
                      // Username
                      TextFormField(
                        controller: _userController,
                        decoration: const InputDecoration(
                          labelText: 'Username',
                          prefixIcon: Icon(Icons.person_outline, size: 20),
                          border: OutlineInputBorder(),
                          hintText: 'admin',
                        ),
                        validator: (val) => (val == null || val.trim().isEmpty) ? 'Required' : null,
                      ),
                      const SizedBox(height: 16),
                      // Password
                      TextFormField(
                        controller: _passController,
                        obscureText: _obscurePassword,
                        decoration: InputDecoration(
                          labelText: 'Password',
                          prefixIcon: const Icon(Icons.lock_outline, size: 20),
                          border: const OutlineInputBorder(),
                          suffixIcon: IconButton(
                            icon: Icon(
                              _obscurePassword ? Icons.visibility_off_outlined : Icons.visibility_outlined,
                              size: 20,
                            ),
                            onPressed: () {
                              setState(() {
                                _obscurePassword = !_obscurePassword;
                              });
                            },
                          ),
                        ),
                        validator: (val) => (val == null || val.isEmpty) ? 'Required' : null,
                        onFieldSubmitted: (_) => _handleConnect(),
                      ),
                      const SizedBox(height: 28),
                      // Connect Button
                      FilledButton(
                        onPressed: _isLoading ? null : _handleConnect,
                        style: FilledButton.styleFrom(
                          padding: const EdgeInsets.symmetric(vertical: 16),
                          shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(10)),
                        ),
                        child: _isLoading
                            ? Row(
                                mainAxisAlignment: MainAxisAlignment.center,
                                children: [
                                  SizedBox(
                                    height: 18,
                                    width: 18,
                                    child: CircularProgressIndicator(
                                      strokeWidth: 2,
                                      color: theme.colorScheme.onPrimary,
                                    ),
                                  ),
                                  const SizedBox(width: 12),
                                  const Text('Connecting...', style: TextStyle(fontSize: 15, fontWeight: FontWeight.bold)),
                                ],
                              )
                            : const Row(
                                mainAxisAlignment: MainAxisAlignment.center,
                                children: [
                                  Icon(Icons.login, size: 18),
                                  SizedBox(width: 8),
                                  Text('Connect to Server', style: TextStyle(fontSize: 15, fontWeight: FontWeight.bold)),
                                ],
                              ),
                      ),
                    ],
                  ),
                ),
              ),
            ),
          ),
        ],
      ),
    );
  }
}
