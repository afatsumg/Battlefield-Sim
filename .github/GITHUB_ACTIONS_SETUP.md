# GitHub Actions CI/CD Configuration

This document describes the GitHub Actions CI/CD pipeline integrated into the Battlefield-Sim project.

## Workflows

### 1. **CI/CD Pipeline** (`ci.yml`)
Main continuous integration pipeline triggered on:
- Push to `master` and `develop` branches
- Pull request creation
- Manual trigger (`workflow_dispatch`)

**Stages:**
1. **Build** - Compiles the C++ project
   - Installs Protobuf v33.2
   - Installs Abseil and gRPC v1.48.0
   - Configures and builds project with CMake
   - Uploads binary artifacts

2. **Docker Build** - Creates and publishes Docker image
   - Image name: `ghcr.io/owner/repository`
   - Tags: branch name, commit SHA, semantic versioning
   - Pushes to GitHub Container Registry (except PRs)

3. **Test** - Runs tests with Docker Compose
   - Sets up Python 3.10 environment
   - Executes simulation suite
   - Uploads test results and logs as artifacts

4. **Code Quality** - Performs code quality checks
   - Linting with flake8
   - Format check with black
   - Import sorting with isort

5. **Security** - Runs Trivy security scan
   - Scans filesystem
   - Generates SARIF format report
   - Uploads to GitHub Security tab

### 2. **Release & Deploy** (`release.yml`)
Triggered on: Git tag push (v*.*.*) or manual trigger

**Stages:**
1. Creates GitHub Release with description
2. Publishes Docker image with semantic version tag
3. Uploads source code as .tar.gz and .zip artifacts
4. Generates and attaches changelog

### 3. **PR Checks** (`pr-checks.yml`)
Special validation for pull requests

**Checks:**
- Commit message format (Conventional Commits)
- File size limits (max 50MB)
- Dependency updates
- Proto and CMakeLists.txt structure
- Docker configuration validation

## Setup

### Step 1: Repository Configuration
1. `.github/workflows/` directory is already created with YAML files
2. Ensure `master` and `develop` branches exist in your repository

### Step 2: GitHub Container Registry Access
For private image publishing:

```bash
# Create a Personal Access Token (PAT):
# 1. GitHub → Settings → Developer settings → Personal access tokens
# 2. Token scopes: write:packages, read:packages

# GitHub Actions automatically uses GITHUB_TOKEN:
# No additional setup required for public repositories
```

### Step 3: Branch Protection Rules (Optional)
To protect the `master` branch in GitHub:

1. Repository Settings → Branches
2. "Add rule" → Branch name pattern: `master`
3. Enable:
   - ✓ Require status checks to pass before merging
   - ✓ Dismiss stale pull request approvals when new commits are pushed
   - ✓ Require code reviews before merging (at least 1)

## Usage

### Creating a Pull Request
```bash
git checkout -b feature/my-feature
# ... make changes ...
git push origin feature/my-feature
# Open PR on GitHub → pr-checks.yml runs automatically
```

### Pushing to master branch
```bash
git push origin master
# ci.yml and docker-build run automatically
```

### Creating a Release
```bash
git tag -a v1.0.0 -m "Release version 1.0.0"
git push origin v1.0.0
# release.yml runs automatically
# Docker image: ghcr.io/owner/repo:v1.0.0
# Binaries uploaded to GitHub Release page
```

## Artifacts

### Build Artifact
- **Name:** `build-artifacts`
- **Contents:** Compiled binary files (`build/services/`)
- **Retention:** 1 day

### Test Artifact
- **Name:** `test-results`
- **Contents:** logs/ and simulation_results/ directories
- **Retention:** 30 days

### Release Artifact
- Source code tarball (.tar.gz)
- Source code archive (.zip)
- CHANGELOG.txt

## Environment Variables

| Variable | Usage | Value |
|----------|-------|-------|
| `REGISTRY` | Docker Registry | `ghcr.io` |
| `IMAGE_NAME` | Docker Image Name | `${{ github.repository }}` |

## Secrets (if needed)

The standard `GITHUB_TOKEN` is automatically provided by GitHub Actions. No additional setup required.

For external registries:
- Repository Settings → Secrets and variables → Actions
- Add `DOCKER_USERNAME` and `DOCKER_PASSWORD`

## Troubleshooting

### Build failures
1. **Dependency errors:** If apt-get update fails on Ubuntu runner
   - Try restarting the runner
   - Check internet connectivity

2. **gRPC installation errors:**
   - Verify version compatibility (protobuf v33.2 + gRPC v1.48.0)

3. **Docker build errors:**
   - Test Dockerfile locally: `docker build -f docker/dev.Dockerfile .`

### Security scan issues
- Review Trivy results: Actions → Security → Code scanning
- Dismiss false positives if needed

### Test timeouts
- Increase `timeout 300` value (in `release.yml`)
- Check test duration parameters

## Best Practices

1. **Branch strategy:**
   - `master` = production-ready
   - `develop` = integration branch
   - Feature branches: `feature/*`, `bugfix/*`, `hotfix/*`

2. **Commit messages:** Use Conventional Commits
   ```
   feat(fusion): implement kalman filter
   fix(sensor): correct radar noise model
   docs: update deployment guide
   ```

3. **Pull requests:**
   - Write descriptive commit messages
   - Explain changes in PR description
   - Add screenshots/videos if applicable

4. **Releases:** Use semantic versioning
   - MAJOR.MINOR.PATCH (v1.0.0)
   - Breaking changes → increment MAJOR
   - New features → increment MINOR
   - Bug fixes → increment PATCH

## Monitoring

On GitHub Actions dashboard:
1. Repository homepage → "Actions" tab
2. View latest workflow runs
3. Click individual runs for detailed logs

For email notifications:
- GitHub → Settings → Notifications → Email
- Enable "Actions" option

## Resources

- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [Docker Build Push Action](https://github.com/docker/build-push-action)
- [Trivy Security Scanner](https://github.com/aquasecurity/trivy-action)
- [Conventional Commits](https://www.conventionalcommits.org/)
