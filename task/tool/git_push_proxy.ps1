<#
.SYNOPSIS
    通过 HTTP 代理把当前分支推送到 GitHub（代理仅用于本次命令，不落盘）。

.DESCRIPTION
    本机 origin 是 SSH 地址（git@github.com:...），但代理只放行 HTTPS、不放行 SSH，
    因此推送必须走 HTTPS URL + 命令级 http.proxy。
    代理对 github.com 会成片返回 502（可持续数分钟），所以内置"探活 + 重试"循环。

.PARAMETER RepoUrl
    HTTPS 仓库地址，默认本仓库。

.PARAMETER Branch
    要推送的分支，默认 master。

.PARAMETER Proxy
    代理地址，默认 172.16.0.11:3128。

.PARAMETER MaxMinutes
    最长等待分钟数，默认 10。

.EXAMPLE
    pwsh -NoProfile -File task/tool/git_push_proxy.ps1
    pwsh -NoProfile -File task/tool/git_push_proxy.ps1 -Branch dev -MaxMinutes 15
#>
param(
    [string]$RepoUrl = 'https://github.com/lishengze/server_9.git',
    [string]$Branch  = 'master',
    [string]$Proxy   = '172.16.0.11:3128',
    [int]$MaxMinutes = 10
)

$ErrorActionPreference = 'Continue'
$env:GIT_TERMINAL_PROMPT = '0'   # 禁止交互式询问，避免卡死
$env:GCM_INTERACTIVE     = 'never'
$proxyUrl = "http://$Proxy"

function Test-ProxyReady {
    $code = (curl.exe -sS -o NUL -w '%{http_code}' --max-time 12 -x $proxyUrl https://github.com/ 2>$null) -join ''
    return ($code -eq '200')
}

$deadline = (Get-Date).AddMinutes($MaxMinutes)
$attempt  = 0
$pushed   = $false

while ((Get-Date) -lt $deadline) {
    $attempt++
    if (-not (Test-ProxyReady)) {
        Write-Host ("[{0}] 第 {1} 次探活：代理暂不可用（502/超时），{2}s 后重试" -f (Get-Date -Format 'HH:mm:ss'), $attempt, 15)
        Start-Sleep -Seconds 15
        continue
    }

    Write-Host ("[{0}] 第 {1} 次推送：{2} -> {3}" -f (Get-Date -Format 'HH:mm:ss'), $attempt, $Branch, $RepoUrl)
    git -c http.proxy=$proxyUrl push $RepoUrl $Branch 2>&1 | ForEach-Object { Write-Host $_ }
    if ($LASTEXITCODE -eq 0) { $pushed = $true; break }
    Start-Sleep -Seconds 15
}

if (-not $pushed) {
    Write-Host "推送失败：代理在 $MaxMinutes 分钟内始终不可用。请稍后重试或向用户确认网络环境。" -ForegroundColor Red
    exit 1
}

# 同步远端跟踪引用（推的是 URL 而非 remote 名，否则 git status 会一直显示 ahead）
git -c http.proxy=$proxyUrl fetch $RepoUrl $Branch 2>&1 | Out-Null
if ($LASTEXITCODE -eq 0) {
    git update-ref "refs/remotes/origin/$Branch" FETCH_HEAD
    Write-Host '已同步远端跟踪引用 origin/'$Branch
}

# 校验：远端 SHA 应等于本地 HEAD
$remote = ((git -c http.proxy=$proxyUrl ls-remote $RepoUrl "refs/heads/$Branch" 2>$null) -join '').Trim()
$local  = (git rev-parse HEAD).Trim()
Write-Host "REMOTE=$remote"
Write-Host "LOCAL =$local"
Write-Host ((git status -sb) | Select-Object -First 1)

# 代理收尾校验：不应存在任何 proxy 配置项
$residue = git config --list --show-origin 2>$null | Select-String -Pattern 'proxy'
if ($residue) {
    Write-Host '警告：检测到 proxy 配置残留，请执行 git config --unset http.proxy / https.proxy' -ForegroundColor Yellow
    $residue | ForEach-Object { Write-Host $_ }
} else {
    Write-Host '代理已取消：无任何 proxy 配置项'
}

if ($remote -like "*$local*") {
    Write-Host '推送校验通过（远端 SHA == 本地 HEAD）' -ForegroundColor Green
    exit 0
} else {
    Write-Host '推送校验失败：远端 SHA 与本地 HEAD 不一致' -ForegroundColor Red
    exit 1
}
