param([int]$oldbuild)

[System.Reflection.Assembly]::LoadWithPartialName("System.Windows.Forms")
[System.Windows.Forms.MessageBox]::Show($oldbuild.ToString(), "Example")
 