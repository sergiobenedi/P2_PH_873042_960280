# Genera MEMORIA_P2_873042_960280.docx y .pdf a partir de memoria_p2.md
# (actualiza la tabla de contenidos con Word y exporta a PDF).
$ErrorActionPreference = "Stop"
$dir = $PSScriptRoot
$nombre = "MEMORIA_P2_873042_960280"
Push-Location $dir
python md2eupt.py memoria_p2.md "$nombre.docx"
Pop-Location

$w = New-Object -ComObject Word.Application
$w.Visible = $false
try {
    $d = $w.Documents.Open("$dir\$nombre.docx")
    $d.TablesOfContents.Item(1).Update()
    $d.Fields.Update() | Out-Null
    $d.TablesOfContents.Item(1).Update()
    $d.Save()
    $d.ExportAsFixedFormat("$dir\$nombre.pdf", 17)
    $d.Close($false)
} finally {
    # Solo cerrar Word si no hay otros documentos abiertos por el usuario
    if ($w.Documents.Count -eq 0) { $w.Quit() }
}
Write-Output "PDF: $dir\$nombre.pdf"
