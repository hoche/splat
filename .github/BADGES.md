# CI/CD Status Badges

Add these badges to your README.md to show build status:

## Main CI Build Status

```markdown
[![CI Build and Test](https://github.com/YOUR_USERNAME/splat/actions/workflows/ci.yml/badge.svg)](https://github.com/YOUR_USERNAME/splat/actions/workflows/ci.yml)
```

## Platform-Specific Badges

### Linux
```markdown
![Linux Build](https://img.shields.io/badge/Linux-passing-brightgreen)
```

### macOS (Intel)
```markdown
![macOS Intel](https://img.shields.io/badge/macOS%20Intel-passing-brightgreen)
```

### macOS (ARM/M1)
```markdown
![macOS ARM](https://img.shields.io/badge/macOS%20ARM-passing-brightgreen)
```

### Windows
```markdown
![Windows](https://img.shields.io/badge/Windows-passing-brightgreen)
```

### FreeBSD
```markdown
![FreeBSD](https://img.shields.io/badge/FreeBSD-passing-brightgreen)
```

## Code Quality Badges

### Code Coverage
```markdown
[![codecov](https://codecov.io/gh/YOUR_USERNAME/splat/branch/main/graph/badge.svg)](https://codecov.io/gh/YOUR_USERNAME/splat)
```

### Static Analysis
```markdown
[![Static Analysis](https://github.com/YOUR_USERNAME/splat/actions/workflows/analysis.yml/badge.svg)](https://github.com/YOUR_USERNAME/splat/actions/workflows/analysis.yml)
```

### Code Coverage Report
```markdown
[![Coverage](https://github.com/YOUR_USERNAME/splat/actions/workflows/coverage.yml/badge.svg)](https://github.com/YOUR_USERNAME/splat/actions/workflows/coverage.yml)
```

## Example README Section

```markdown
# SPLAT! v2.0

[![CI Build and Test](https://github.com/YOUR_USERNAME/splat/actions/workflows/ci.yml/badge.svg)](https://github.com/YOUR_USERNAME/splat/actions/workflows/ci.yml)
[![codecov](https://codecov.io/gh/YOUR_USERNAME/splat/branch/main/graph/badge.svg)](https://codecov.io/gh/YOUR_USERNAME/splat)
[![Static Analysis](https://github.com/YOUR_USERNAME/splat/actions/workflows/analysis.yml/badge.svg)](https://github.com/YOUR_USERNAME/splat/actions/workflows/analysis.yml)

![Linux](https://img.shields.io/badge/Linux-passing-brightgreen)
![macOS Intel](https://img.shields.io/badge/macOS%20Intel-passing-brightgreen)
![macOS ARM](https://img.shields.io/badge/macOS%20ARM-passing-brightgreen)
![Windows](https://img.shields.io/badge/Windows-passing-brightgreen)
![FreeBSD](https://img.shields.io/badge/FreeBSD-passing-brightgreen)

An RF Signal Propagation, Loss, And Terrain analysis tool.
```

## Notes

- Replace `YOUR_USERNAME` with your GitHub username
- Replace `main` with your default branch name if different
- The badges will automatically update based on your latest workflow runs
- You can customize badge colors and text using shields.io syntax
