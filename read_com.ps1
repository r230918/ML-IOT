$port = New-Object System.IO.Ports.SerialPort "COM10", 115200
$port.ReadTimeout = 2000
try {
    $port.Open()
    Start-Sleep -Milliseconds 600
    $bytesToRead = $port.BytesToRead
    Write-Host "Bytes available to read: $bytesToRead"
    if ($bytesToRead -gt 0) {
        $buf = New-Object byte[] $bytesToRead
        $readCount = $port.Read($buf, 0, $bytesToRead)
        Write-Host "Bytes read: $readCount"
        $str = [System.Text.Encoding]::ASCII.GetString($buf)
        Write-Host "Raw Text output:"
        Write-Host $str
    }
    $port.Close()
} catch {
    Write-Host "Error: $_"
}
