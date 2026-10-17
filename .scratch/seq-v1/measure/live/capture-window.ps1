# Captures one window of Live by its title, through PrintWindow: that window alone, never the screen (a screen copy
# once caught the user's browser). Writes a PNG.
#   powershell -File capture-window.ps1 -Title "MD Drums Seq: Edit" -Out C:\path\window.png
param([Parameter(Mandatory)][string]$Title, [Parameter(Mandatory)][string]$Out)

Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
public static class Win
{
	public delegate bool EnumProc(IntPtr hwnd, IntPtr lParam);
	[DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc proc, IntPtr lParam);
	[DllImport("user32.dll")] public static extern int GetWindowText(IntPtr hwnd, StringBuilder text, int max);
	[DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hwnd);
	[DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hwnd, out uint pid);
	[DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hwnd, out RECT rect);
	[DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hwnd, IntPtr hdc, uint flags);
	[StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
}
"@

$live = Get-Process | Where-Object { $_.ProcessName -like "Ableton Live*" } | Select-Object -First 1
if(-not $live) { throw "Live is not running" }
$found = [IntPtr]::Zero
$callback = [Win+EnumProc]{
	param($hwnd, $lParam)
	$procId = 0
	[void][Win]::GetWindowThreadProcessId($hwnd, [ref]$procId)
	if($procId -ne $live.Id -or -not [Win]::IsWindowVisible($hwnd)) { return $true }
	$text = New-Object System.Text.StringBuilder 256
	[void][Win]::GetWindowText($hwnd, $text, 256)
	if($text.ToString() -eq $Title) { $script:found = $hwnd; return $false }
	return $true
}
[void][Win]::EnumWindows($callback, [IntPtr]::Zero)
if($found -eq [IntPtr]::Zero) { throw "no window of Live titled '$Title'" }
$rect = New-Object Win+RECT
[void][Win]::GetWindowRect($found, [ref]$rect)
$bitmap = New-Object System.Drawing.Bitmap ($rect.Right - $rect.Left), ($rect.Bottom - $rect.Top)
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
$hdc = $graphics.GetHdc()
[void][Win]::PrintWindow($found, $hdc, 2)	# PW_RENDERFULLCONTENT
$graphics.ReleaseHdc($hdc)
$bitmap.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
"captured $($bitmap.Width) x $($bitmap.Height) to $Out"
