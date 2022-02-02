& {$outpath = ($Env:CODE_SIGNING_PFX_PATH);
    $inpath = ($Env:CODE_SIGNING_PFX_PATH);
    [IO.File]::WriteAllBytes($outpath, ([convert]::FromBase64String(([IO.File]::ReadAllText($inpath)))))}
