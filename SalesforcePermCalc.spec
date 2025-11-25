# -*- mode: python ; coding: utf-8 -*-


a = Analysis(
    ['perm_set_calculator.py'],
    pathex=[],
    binaries=[],
    datas=[('Salesforce_perm_Calc_icon.ico', '.'), ('Salesforce_perm_Calc_icon.png', '.')],
    hiddenimports=[],
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=[],
    noarchive=False,
    optimize=0,
)
pyz = PYZ(a.pure)

exe = EXE(
    pyz,
    a.scripts,
    a.binaries,
    a.datas,
    [],
    name='SalesforcePermCalc',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    upx_exclude=[],
    runtime_tmpdir=None,
    console=False,
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
    icon=['Salesforce_perm_Calc_icon.ico'],
)
