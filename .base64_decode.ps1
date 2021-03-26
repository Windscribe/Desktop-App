& {$outpath = ($Env:CODE_SIGNING_PFX_PATH); $inpath = ($Env:CODE_SIGNING_PFX_BASE64); [IO.File]::WriteAllBytes($outpath, ([convert]::FromBase64String(([IO.File]::ReadAllText($inpath)))))}
