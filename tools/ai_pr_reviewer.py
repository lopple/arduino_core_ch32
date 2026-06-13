import os
import subprocess
import urllib.request
import urllib.parse
import urllib.error
import json

def run_cmd(cmd):
    res = subprocess.run(cmd, capture_output=True, encoding="utf-8", errors="ignore")
    if res.returncode != 0:
        raise RuntimeError(f"Command failed: {' '.join(cmd)}\nStderr: {res.stderr}")
    return res.stdout.strip()

def main():
    gemini_key = os.environ.get("GEMINI_API_KEY")
    github_token = os.environ.get("GITHUB_TOKEN")
    event_path = os.environ.get("GITHUB_EVENT_PATH")
    repo = os.environ.get("GITHUB_REPOSITORY")

    if not gemini_key:
        print("Error: GEMINI_API_KEY is not set.")
        return
    if not github_token:
        print("Error: GITHUB_TOKEN is not set.")
        return
    if not event_path:
        print("Error: GITHUB_EVENT_PATH is not set.")
        return

    # Load event payload
    with open(event_path, "r", encoding="utf-8") as f:
        event = json.load(f)

    gh_headers = {
        "Authorization": f"Bearer {github_token}",
        "Accept": "application/vnd.github.v3+json",
        "User-Agent": "GitHub-Actions-AI-PR-Reviewer"
    }

    pr_number = event.get("number")
    if not pr_number:
        print("PR number not found in event payload, searching for open PRs for this branch...")
        try:
            branch_name = os.environ.get("GITHUB_REF_NAME")
            if not branch_name or branch_name == "HEAD":
                branch_name = run_cmd(["git", "rev-parse", "--abbrev-ref", "HEAD"])
            owner_repo = repo.split("/")
            gh_url = f"https://api.github.com/repos/{repo}/pulls?head={owner_repo[0]}:{branch_name}&state=open"
            gh_req = urllib.request.Request(gh_url, headers=gh_headers, method="GET")
            with urllib.request.urlopen(gh_req) as res:
                pulls = json.loads(res.read().decode("utf-8"))
                if pulls:
                    pr_number = pulls[0]["number"]
                    print(f"Found active PR #{pr_number} for branch {branch_name}")
                else:
                    print(f"No open PR found for branch {branch_name}, skipping review.")
                    return
        except Exception as e:
            print(f"Failed to query GitHub PRs: {e}")
            return

    if "pull_request" in event:
        base_ref = event["pull_request"]["base"]["ref"]
    else:
        print(f"Fetching PR #{pr_number} details to get base ref...")
        try:
            pr_url = f"https://api.github.com/repos/{repo}/pulls/{pr_number}"
            pr_req = urllib.request.Request(pr_url, headers=gh_headers, method="GET")
            with urllib.request.urlopen(pr_req) as res:
                pr_data = json.loads(res.read().decode("utf-8"))
                base_ref = pr_data["base"]["ref"]
        except Exception as e:
            print(f"Failed to fetch PR base ref: {e}")
            return
    
    # Fetch base branch to generate diff
    print(f"Fetching base branch origin/{base_ref}...")
    run_cmd(["git", "fetch", "origin", base_ref])
    
    # Get diff
    print("Generating diff...")
    diff_text = run_cmd(["git", "diff", f"origin/{base_ref}...HEAD"])
    
    if not diff_text:
        print("No code changes detected.")
        return

    # Truncate diff if it's too large to prevent hitting token limits
    if len(diff_text) > 40000:
        diff_text = diff_text[:40000] + "\n\n(Diff truncated due to size limits...)"

    prompt = (
        "あなたは組み込み C/C++ および 32ビット RISC-V マイコン（CH32シリーズなど）の超一流のファームウェアセキュリティ＆品質監査専門家です。\n"
        "提示された Pull Request のコード変更差分（diff）を監査し、コードに潜在する以下のリスクを検出してください。\n\n"
        "特に重点的にチェックすべき項目：\n"
        "1. **32ビットマイコンにおける64ビット変数の非アトミック読み書き**:\n"
        "   - `uint64_t` や `unsigned long long` などの変数が、割り込みハンドラ（ISR）とメインループ等の異なるコンテキストで共有されている場合、アトミックに読み書きされていない（値の破損やロールオーバー時のズレが生じる）バグがないか。\n"
        "2. **共有変数の volatile 指定漏れ**:\n"
        "   - 割り込みハンドラ内で更新され、メインループ等の通常処理側でポーリング/参照される変数に `volatile`（または `__IO`）指定が漏れていないか（コンパイラ最適化による無限ループや値のキャッシュバグを防群ため）。\n"
        "3. **割り込み排他制御（クリティカルセクション）の不足**:\n"
        "   - 複数コンテキストで共有されるリソースへのアクセスに対し、適切に割り込み禁止（`__disable_irq()` / `__enable_irq()`）やロックフリーな対策が施されているか。\n"
        "4. **不要な64ビット演算によるバイナリサイズ肥大化**:\n"
        "   - 64ビットの除算（`/`）や剰余（`%`）演算が追加され、大容量のソフトウェアヘルパーライブラリがリンクされてしまうような非効率な記述がないか。\n\n"
        f"差分（diff）:\n{diff_text}\n\n"
        "---\n"
        "レビュー結果を日本語でわかりやすく報告してください。\n"
        "問題が検出された場合は、ファイル名・行番号・問題の解説・および修正提案のコード差分（diff形式）を記述してください。\n"
        "特に問題が検出されなかった場合は、「今回の変更にはアトミック性や割り込み競合に関する潜在リスクは見つかりませんでした。安全なコードです。」と短く回答してください。"
    )

    # Call Gemini API
    print("Calling Gemini API...")
    url = f"https://generativelanguage.googleapis.com/v1/models/gemini-1.5-flash:generateContent?key={gemini_key}"
    headers = {"Content-Type": "application/json"}
    data = {
        "contents": [{
            "parts": [{
                "text": prompt
            }]
        }]
    }

    req = urllib.request.Request(
        url,
        data=json.dumps(data).encode("utf-8"),
        headers=headers,
        method="POST"
    )

    try:
        with urllib.request.urlopen(req) as res:
            res_body = json.loads(res.read().decode("utf-8"))
            review_comment = res_body["candidates"][0]["content"]["parts"][0]["text"]
    except urllib.error.HTTPError as e:
        print(f"Failed to query Gemini API (HTTPError): {e}")
        try:
            print(f"Error response body: {e.read().decode('utf-8')}")
        except Exception as read_err:
            print(f"Failed to read error body: {read_err}")
        return
    except Exception as e:
        print(f"Failed to query Gemini API: {e}")
        return

    # Post comment to GitHub PR
    print("Posting review comment to GitHub...")
    gh_url = f"https://api.github.com/repos/{repo}/issues/{pr_number}/comments"
    gh_headers = {
        "Authorization": f"Bearer {github_token}",
        "Accept": "application/vnd.github.v3+json",
        "Content-Type": "application/json",
        "User-Agent": "GitHub-Actions-AI-PR-Reviewer"
    }
    gh_data = {
        "body": (
            "🤖 **AI ファームウェア監査レビュー (Gemini)**\n\n"
            f"{review_comment}"
        )
    }

    gh_req = urllib.request.Request(
        gh_url,
        data=json.dumps(gh_data).encode("utf-8"),
        headers=gh_headers,
        method="POST"
    )

    try:
        with urllib.request.urlopen(gh_req) as res:
            print("Comment posted successfully!")
    except Exception as e:
        print(f"Failed to post comment to GitHub: {e}")

# Trigger PR review test run
if __name__ == "__main__":
    main()
