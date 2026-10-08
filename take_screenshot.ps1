param([int]$screenIndex, [string]$outName)
$body = "{`"index`":$screenIndex}"
Invoke-RestMethod -Uri "http://192.168.0.2/api/screen" -Method Post -Body $body -ContentType "application/json"
Start-Sleep -Seconds 3
$bmpPath = Join-Path $PSScriptRoot "$outName.bmp"
$pngPath = Join-Path $PSScriptRoot "$outName.png"
Invoke-WebRequest -Uri "http://192.168.0.2/api/screenshot.bmp" -OutFile $bmpPath
Add-Type -AssemblyName System.Drawing
$image = [System.Drawing.Image]::FromFile($bmpPath)
$image.Save($pngPath, [System.Drawing.Imaging.ImageFormat]::Png)
$image.Dispose()
Write-Output "Saved $pngPath"
