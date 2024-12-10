Write-Output "Cooking Game"
Start-Process -filepath "Binaries\Editor\Win64\Development\FlaxEditor.exe" -Wait -NoNewWindow -PassThru -ArgumentList '-std -headless -mute -null -project "FlaxSamples/MaterialsFeaturesTour" -build "Development.Windows"'

Write-Output "Testing Game"
Start-Process -filepath "FlaxSamples\MaterialsFeaturesTour\Output\Windows\MaterialsFeaturesTour.exe" -Wait -NoNewWindow -PassThru -ArgumentList '-std -headless -mute -null'
