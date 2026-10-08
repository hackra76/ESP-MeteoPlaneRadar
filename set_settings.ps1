param([string]$json)
$bytes = [System.Text.Encoding]::UTF8.GetBytes($json)
Invoke-RestMethod -Uri "http://192.168.0.2/api/config" -Method Post -Body $bytes -ContentType "application/json"
