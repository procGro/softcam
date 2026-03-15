param (
    [string]$FriendlyName = "Integrated Webcam",
    [string]$NewGUID = $([guid]::NewGuid().ToString().ToUpper())
)

$softcam_cpp = "..\..\src\softcam\softcam.cpp"
$content = Get-Content $softcam_cpp

# Create custom DEFINE_GUID string
# e.g. AEF3B972-5FA5-4647-9571-358EB472BC9E
$g_parts = $NewGUID.Split('-')
$g_data1 = "0x" + $g_parts[0]
$g_data2 = "0x" + $g_parts[1]
$g_data3 = "0x" + $g_parts[2]
$g_d4 = $g_parts[3] + $g_parts[4]
$g_d4_parts = @()
for ($i=0; $i -lt 16; $i+=2) {
    $g_d4_parts += "0x" + $g_d4.Substring($i, 2)
}
$g_d4_str = [string]::Join(", ", $g_d4_parts)

$new_define_guid = "DEFINE_GUID(CLSID_DShowSoftcam,`n$g_data1, $g_data2, $g_data3, $g_d4_str);"
$new_filter_name = "const wchar_t FILTER_NAME[] = L""$FriendlyName"";"

# Replace GUID
$content = $content -replace 'DEFINE_GUID\(CLSID_DShowSoftcam,[\s\S]*?\);', $new_define_guid
# Replace FriendlyName
$content = $content -replace 'const wchar_t FILTER_NAME\[\] = L"DirectShow Softcam";', $new_filter_name
# Also replace the comment with the original GUID for consistency
$content = $content -replace '// {AEF3B972-5FA5-4647-9571-358EB472BC9E}', "// {$NewGUID}"

Set-Content $softcam_cpp $content
Write-Host "Replaced CLSID with {$NewGUID} and FriendlyName with '$FriendlyName' in softcam.cpp"
