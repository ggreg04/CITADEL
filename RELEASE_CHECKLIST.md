# 🚀 CITADEL Release Checklist

Follow these steps in order every time you cut a new release.

---

## Step 1 — Finalize Code in Claude Code

```bash
# Make sure you're on main and up to date
git checkout main
git pull origin main

# Stage all changes
git add .

# Commit with a descriptive message
git commit -m "release: vX.X.X — [brief summary of what changed]"

# Push to GitHub
git push origin main
```

---

## Step 2 — Verify Push on GitHub

- Go to https://github.com/YOUR_USERNAME/CITADEL
- Confirm your latest commit appears on the `main` branch
- Quick-check the `.ino` file looks correct

---

## Step 3 — Run the Release Workflow

1. Go to your repo → **Actions** tab
2. Select **"🚀 CITADEL Release"** from the left sidebar
3. Click **"Run workflow"** (top right)
4. Fill in the inputs:
   - **Version:** `vMAJOR.MINOR.PATCH` (e.g. `v2.1.1`)
   - **Release notes:** What changed, what was fixed
   - **Pre-release:** Leave unchecked unless testing
5. Click **"Run workflow"** green button

---

## Step 4 — Confirm Release

- Watch the workflow run (takes ~30 seconds)
- Go to **Releases** tab and confirm the new release appears
- Verify the `.zip` file is attached and downloadable
- Check the auto-generated install instructions look correct

---

## Version Numbering Guide

| Change Type        | Example             | When to use                          |
|--------------------|---------------------|--------------------------------------|
| Patch `vX.X.+1`   | `v2.1.0 → v2.1.1`  | Bug fixes, small tweaks              |
| Minor `vX.+1.0`   | `v2.1.1 → v2.2.0`  | New features, non-breaking           |
| Major `v+1.0.0`   | `v2.2.0 → v3.0.0`  | Major rewrites, breaking changes     |

---

## Troubleshooting

- **"Tag already exists" error** → Bump your version number, you've already released that one.
- **"Invalid version format" error** → Must be exactly `vX.X.X` — don't forget the `v` prefix.
- **Workflow not showing in Actions tab** → Make sure `release.yml` is in `.github/workflows/` and pushed to `main`.
