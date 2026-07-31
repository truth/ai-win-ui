<#
.SYNOPSIS
  Generate samples/sticky_notes/app.ico from the same "folded note" artwork
  used for the runtime tray icon (see CreateNoteIcon in main.cpp).

.DESCRIPTION
  Draws the icon at 16/32/48/256 px with GDI+, embeds each size as PNG inside
  a hand-written ICO container (PNG-in-ICO is valid since Windows Vista), so
  the compiled sticky_notes.exe carries a real file icon instead of the
  Windows default.

.EXAMPLE
  .\scripts\gen_sticky_notes_icon.ps1
#>
param(
    [string]$OutFile = "samples\sticky_notes\app.ico"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

Add-Type -AssemblyName System.Drawing

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$ResolvedOut = Join-Path $RepoRoot $OutFile

function New-NoteIconBitmap {
    param([int]$Size)

    $bmp = New-Object System.Drawing.Bitmap($Size, $Size, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.Clear([System.Drawing.Color]::Transparent)

    $inset = [Math]::Max(1, [int]($Size / 8))
    $fold = [Math]::Max(2, [int]($Size / 4))
    $x0 = $inset
    $y0 = $inset
    $w = $Size - 2 * $inset
    $h = $Size - 2 * $inset
    $radius = [Math]::Max(1, [int]($Size / 10))

    # Folded-corner note outline (card minus a top-right triangular "dog-ear").
    $path = New-Object System.Drawing.Drawing2D.GraphicsPath
    $path.AddLine($x0 + $radius, $y0, $x0 + $w - $fold, $y0)
    $path.AddLine($x0 + $w - $fold, $y0, $x0 + $w, $y0 + $fold)
    $path.AddLine($x0 + $w, $y0 + $fold, $x0 + $w, $y0 + $h - $radius)
    $path.AddArc($x0 + $w - 2 * $radius, $y0 + $h - 2 * $radius, 2 * $radius, 2 * $radius, 0, 90)
    $path.AddLine($x0 + $w - $radius, $y0 + $h, $x0 + $radius, $y0 + $h)
    $path.AddArc($x0, $y0 + $h - 2 * $radius, 2 * $radius, 2 * $radius, 90, 90)
    $path.AddLine($x0, $y0 + $h - $radius, $x0, $y0 + $radius)
    $path.AddArc($x0, $y0, 2 * $radius, 2 * $radius, 180, 90)
    $path.CloseFigure()

    $body = [System.Drawing.Color]::FromArgb(255, 255, 238, 156)   # warm yellow
    $bar = [System.Drawing.Color]::FromArgb(255, 240, 193, 75)     # amber header
    $ink = [System.Drawing.Color]::FromArgb(255, 154, 123, 16)     # ink lines
    $foldShade = [System.Drawing.Color]::FromArgb(255, 214, 178, 96)

    $brush = New-Object System.Drawing.SolidBrush($body)
    $g.FillPath($brush, $path)
    $g.SetClip($path)

    $barH = [Math]::Max(1, [int]($h / 5))
    $g.FillRectangle((New-Object System.Drawing.SolidBrush($bar)), $x0, $y0, $w, $barH)

    # Folded corner shading triangle.
    $foldPts = @(
        (New-Object System.Drawing.Point(($x0 + $w - $fold), $y0)),
        (New-Object System.Drawing.Point(($x0 + $w), ($y0 + $fold))),
        (New-Object System.Drawing.Point(($x0 + $w - $fold), ($y0 + $fold)))
    )
    $g.FillPolygon((New-Object System.Drawing.SolidBrush($foldShade)), $foldPts)

    if ($Size -ge 24) {
        $pen = New-Object System.Drawing.Pen($ink, [Math]::Max(1, [int]($Size / 32)))
        $lineY = $y0 + $barH + [int](($h - $barH) / 4)
        $step = [Math]::Max(1, [int](($h - $barH) / 3))
        for ($i = 0; $i -lt 3; $i++) {
            $ly = $lineY + $i * $step
            if ($ly -lt ($y0 + $h - 4)) {
                $g.DrawLine($pen, $x0 + 4, $ly, $x0 + $w - 6, $ly)
            }
        }
    }

    $g.ResetClip()
    $g.Dispose()
    return $bmp
}

function ConvertTo-PngBytes {
    param([System.Drawing.Bitmap]$Bitmap)
    $ms = New-Object System.IO.MemoryStream
    $Bitmap.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
    $bytes = $ms.ToArray()
    $ms.Dispose()
    # The comma operator prevents PowerShell from unrolling the byte array into
    # the pipeline (which would silently degrade it to a boxed System.Object[]
    # and corrupt BinaryWriter.Write(byte[]) into writing 1 byte per call).
    return ,$bytes
}

$sizes = @(16, 32, 48, 256)
$images = @()
foreach ($s in $sizes) {
    $bmp = New-NoteIconBitmap -Size $s
    $png = ConvertTo-PngBytes -Bitmap $bmp
    $bmp.Dispose()
    $images += [PSCustomObject]@{ Size = $s; Png = $png }
}

$fs = [System.IO.File]::Open($ResolvedOut, [System.IO.FileMode]::Create)
$w = New-Object System.IO.BinaryWriter($fs)

# ICONDIR
$w.Write([uint16]0)          # reserved
$w.Write([uint16]1)          # type = icon
$w.Write([uint16]$images.Count)

$headerSize = 6 + 16 * $images.Count
$offset = $headerSize
foreach ($img in $images) {
    $dim = if ($img.Size -ge 256) { 0 } else { $img.Size } # 0 means 256 per ICO spec
    $w.Write([byte]$dim)          # width
    $w.Write([byte]$dim)          # height
    $w.Write([byte]0)             # color count
    $w.Write([byte]0)             # reserved
    $w.Write([uint16]1)           # color planes
    $w.Write([uint16]32)          # bits per pixel
    $w.Write([uint32]$img.Png.Length)
    $w.Write([uint32]$offset)
    $offset += $img.Png.Length
}
foreach ($img in $images) {
    $w.Write($img.Png)
}

$w.Flush()
$w.Dispose()
$fs.Dispose()

Write-Host "Wrote $ResolvedOut ($($images.Count) sizes: $($sizes -join ', '))"
