#!/bin/bash
# deploy.sh — Deploy OpenMind to static hosting
set -e

echo "=== OpenMind Deploy ==="

DIST_DIR="build/dist"

# Check if build exists
if [ ! -d "$DIST_DIR" ]; then
    echo "Build not found. Running build_web.sh first..."
    bash build/build_web.sh
fi

echo ""
echo "Deployment options:"
echo "  1. Netlify (drag & drop $DIST_DIR)"
echo "  2. Vercel (npx vercel $DIST_DIR)"
echo "  3. Cloudflare Pages (wrangler pages deploy $DIST_DIR)"
echo "  4. GitHub Pages (copy to docs/ folder)"
echo "  5. Local server (python -m http.server 8080 -d $DIST_DIR)"
echo ""

# Auto-detect available tools
if command -v npx &> /dev/null; then
    echo "Detected npx. Deploy with Vercel? (y/n)"
    read -r answer
    if [ "$answer" = "y" ]; then
        npx vercel "$DIST_DIR" --yes
        exit 0
    fi
fi

if command -v wrangler &> /dev/null; then
    echo "Detected wrangler. Deploy to Cloudflare Pages? (y/n)"
    read -r answer
    if [ "$answer" = "y" ]; then
        wrangler pages deploy "$DIST_DIR" --project-name=openmind
        exit 0
    fi
fi

echo "Manual deployment: upload $DIST_DIR/ to your hosting provider."
echo "For local testing: python -m http.server 8080 -d $DIST_DIR"
