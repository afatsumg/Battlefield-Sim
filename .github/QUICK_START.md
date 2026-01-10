# CI/CD Quick Start Guide

## TL;DR - Quick Start

GitHub Actions CI/CD system is now set up. You can run the following commands:

### 1. Opening a PR (Tests run automatically)
```bash
git checkout -b feature/my-feature
# ... make changes ...
git add .
git commit -m "feat: describe your feature"
git push origin feature/my-feature
# Open PR on GitHub → Automatic tests run
```

### 2. Push to master branch (CI runs)
```bash
git push origin master
# Automatic: build + test + publish docker image
```

### 3. Create a Release (Version tag)
```bash
git tag -a v1.0.0 -m "Release v1.0.0"
git push origin v1.0.0
# Automatic: publish Docker + create Release page
```

## Workflows

| File | Trigger | What it does |
|------|---------|----------|
| `ci.yml` | Push to main/develop, PR | Build + Test + Docker |
| `release.yml` | Tag push (v*.*.*) | Create Release + Publish Docker |
| `pr-checks.yml` | PR opens | Code quality checks |

## What you'll see on GitHub

1. **Actions Tab** → All workflows and their status
2. **Packages** → Docker images (ghcr.io)
3. **Releases** → Version releases and binaries
4. **Security** → Trivy scan results

## Pulling Docker Image

```bash
# Use image from master branch
docker pull ghcr.io/your-username/battlefield-sim:master

# Or specific release
docker pull ghcr.io/your-username/battlefield-sim:v1.0.0
```

## Where files went

- `.github/workflows/ci.yml` - Main CI pipeline
- `.github/workflows/release.yml` - Release pipeline  
- `.github/workflows/pr-checks.yml` - PR validation
- `.github/GITHUB_ACTIONS_SETUP.md` - Detailed configuration docs

## Debugging

**Workflow failed?**
1. Open Actions tab
2. Click the failed run
3. Read the logs (usually very descriptive)
4. Check `dev.Dockerfile` and `CMakeLists.txt`

**Docker push error?**
- GitHub Container Registry works automatically
- For private repos, `GITHUB_TOKEN` might be insufficient

**Test timeout?**
- Increase `timeout 300` value in `ci.yml`

**Note:** This project uses `master` branch instead of `main`

## Next Steps

1. ✅ CI/CD is set up - all done this time
2. ⬜ Add branch protection rules (optional)
3. ⬜ Add Slack/email notifications (optional)
4. ⬜ Add performance metrics dashboard (advanced)

---

**Questions?** Read `.github/GITHUB_ACTIONS_SETUP.md` or check [GitHub Actions docs](https://docs.github.com/en/actions).
