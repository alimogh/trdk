$rootPath = $PSScriptRoot + '\..\'
$llvmCmd = 'C:\"Program Files"\LLVM\bin\clang-format.exe -i -style=file -fallback-style=none -assume-filename="{0}" "{0}"'
Get-ChildItem -Path ($rootPath + 'sources') -Recurse -Include @("*.h", "*.c", "*.hpp", "*.cpp") | 
Foreach-Object {
  $line = $llvmCmd -f $_.FullName
  Write-Output $line
}