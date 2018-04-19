[Setup]
;-- Main Setup Information
 AppName                = CRC32 Calculator
 AppVerName             = CRC32 Calculator 1.0.0.1
 AppCopyright           = Copyright © 1995-2009 NetworkDLS.
 DefaultDirName         = {pf}\NetworkDLS\CRC32 Calculator
 DefaultGroupName       = NetworkDLS\CRC32 Calculator
 UninstallDisplayIcon   = {app}\CRC32Calc.exe
 WizardImageFile        = \..\..\@Resources\Setup\LgSetup.bmp
 WizardSmallImageFile   = \..\..\@Resources\Setup\SmSetup.bmp
 PrivilegesRequired     = PowerUser
 Uninstallable          = Yes
 AppMutex               = CRC32 Calculator
 LicenseFile            = \..\..\@Resources\Setup\EULA.txt
 Compression            = ZIP/9
 OutputBaseFilename     = Setup
 MinVersion             = 0.0,5.0

;-- Windows 2000 & XP (Support Dialog)
 AppPublisher    = NetworkDLS
 AppPublisherURL = http://www.NetworkDLS.com/
 AppUpdatesURL   = http://www.NetworkDLS.com/
 AppVersion      = 1.0.0.1

[Files]
 Source: "..\Release\CRC32Calc.exe"; DestDir: "{app}";

[Icons]
 Name: "{group}\CRC32 Calculator";  Filename: "{app}\CRC32Calc.Exe";

[Run]
 Filename: "{app}\CRC32Calc.Exe"; Description: "Launch application"; Flags: postinstall nowait skipifsilent unchecked

