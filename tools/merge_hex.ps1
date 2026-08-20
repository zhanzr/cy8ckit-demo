# merge_hex.ps1 - merge the cat1cm0p part (0x10000000..0x1001FFFF) of $cm0pHex
# with the CM4 app part (0x10020000+) of $appHex into $outHex.
param(
    [string]$cm0pHex,
    [string]$appHex,
    [string]$outHex
)
$out = New-Object System.Collections.Generic.List[string]
$base = 0
foreach ($l in (Get-Content $cm0pHex)) {
    if ($l -match "^:") {
        $typ = [Convert]::ToInt32($l.Substring(7,2),16)
        if ($typ -eq 4) { $base = [Convert]::ToInt32($l.Substring(9,4),16) }
        elseif ($typ -eq 0) {
            $addr = $base * 65536 + [Convert]::ToInt32($l.Substring(3,4),16)
            if ($addr -ge 0x10020000) { break }   # stop at the CM4 region
        }
        $out.Add($l)
    }
}
foreach ($l in (Get-Content $appHex)) {
    if ($l -match "^:") { $out.Add($l) }
}
$out.Add(":00000001FF")
Set-Content -Path $outHex -Value $out.ToArray() -Encoding ASCII
Write-Output "merged -> $outHex ($($out.Count) lines)"
