$sqlqueryDir = "$env:ALLUSERSPROFILE\chocolatey\lib\sqlquery"
# $sqlqueryExe = "$sqlqueryDir\tools\SQLQueryAnalyzer.exe"
# Install-ChocolateyFileAssociation ".sdf" $sqlqueryExe

cmd /c assoc .sqlite=sqlitedbfile
cmd /c ftype sqlitedbfile=SQLQueryAnalyzer.exe -File `"SQLQueryAnalyzer.exe`" `"%1`"

$WshShell = New-Object -comObject WScript.Shell
$Shortcut = $WshShell.CreateShortcut("$Home\Desktop\SQL Query Analyzer.lnk")
$Shortcut.TargetPath = "$sqlqueryDir\tools\SQLQueryAnalyzer.exe"
$Shortcut.Save()
