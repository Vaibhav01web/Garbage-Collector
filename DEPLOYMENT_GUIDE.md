# Cloud Hosting Guide — Memory Compaction & GC Engine

This guide walks you through deploying your C++ Memory Compaction & Garbage Collector Engine to the cloud for free using Docker.

---

## Prerequisites: Push to GitHub

All cloud platforms (Render, Railway, Fly.io) build and deploy directly from your GitHub repository.

1. Create a new repository on [GitHub](https://github.com/new) named `gc-engine` (public or private).
2. Run the following commands in your terminal:

```bash
# 1. Add all project files
git add .

# 2. Create the first commit
git commit -m "Initial release: Memory Compaction & GC Engine"

# 3. Rename branch to main
git branch -M main

# 4. Link your GitHub repository (replace with your actual GitHub username)
git remote add origin https://github.com/YOUR_USERNAME/gc-engine.git

# 5. Push to GitHub
git push -u origin main
```

---

## Option 1: Render (Recommended — Free Tier, No Credit Card Required)

[Render](https://render.com) provides free container hosting with automated HTTPS and GitHub integration.

1. Sign up / Log in to [Render](https://dashboard.render.com/).
2. Click the **"New +"** button at the top right and select **"Web Service"**.
3. Choose **"Build and deploy from a Git repository"** and select your `gc-engine` repository.
4. Render will automatically detect the `Dockerfile`:
   - **Name**: `gc-engine` (or your preferred name)
   - **Region**: Choose the closest region (e.g., Oregon or Frankfurt)
   - **Environment**: `Docker`
   - **Instance Type**: Select **"Free"**
5. Click **"Deploy Web Service"** at the bottom.
6. Render builds the lightweight Alpine Linux container and launches your app.
7. Once deployed, you will get a permanent public URL, such as:
   ```
   https://gc-engine.onrender.com
   ```

---

## Option 2: Railway

[Railway](https://railway.app) provides ultra-fast deployments with automatic Dockerfile detection.

1. Sign up / Log in at [railway.app](https://railway.app/).
2. Click **"New Project"** &rarr; **"Deploy from GitHub repo"**.
3. Select your `gc-engine` repository.
4. Railway will read [railway.json](railway.json) and [Dockerfile](Dockerfile) and build your service automatically.
5. In **Settings** &rarr; **Networking**, click **"Generate Domain"** to get your public URL (e.g. `https://gc-engine.up.railway.app`).

---

## Option 3: Fly.io (Command-Line Deployment)

[Fly.io](https://fly.io) runs Docker containers on global edge servers.

1. Install the Fly CLI:
   ```bash
   brew install flyctl
   ```
2. Log in:
   ```bash
   fly auth login
   ```
3. Deploy directly using the pre-configured [fly.toml](fly.toml):
   ```bash
   fly launch
   ```
   (Accept the detected configuration and choose `Yes` to deploy).
4. View your live application:
   ```bash
   fly open
   ```

---

## How It Works in Production

- **Multi-Stage Dockerfile**: Builds on Alpine Linux with `g++` and packages into a runtime image under **15 MB**.
- **Dynamic Port Assignment**: `src/main.cpp` checks `std::getenv("PORT")`, so it seamlessly binds to whichever internal port the cloud host assigns (e.g. `10000` on Render, `8080` on Fly, or dynamic on Railway).
- **Health Checks**: Configured to route to `GET /api/health` so cloud orchestrators verify the engine is healthy before routing traffic.
- **Static Assets & PDF**: The web visualizer and PDF documentation (`/docs.pdf`) are served directly by the C++ engine in the container with zero extra web servers needed.
