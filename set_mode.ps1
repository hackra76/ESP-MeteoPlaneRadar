param([int]$sonarMode = -1, [int]$issMode = -1)
$dict = @{}
if ($sonarMode -ge 0) { $dict["sonarViewMode"] = $sonarMode }
if ($issMode -ge 0) { $dict["issViewMode"] = $issMode }
$json = $dict | ConvertTo-Json -Compress
$bytes = [System.Text.Encoding]::UTF8.GetBytes($json)
Invoke-RestMethod -Uri "http://192.168.0.2/api/config" -Method Post -Body $bytes -ContentType "application/json"
