# Wiki sources

These Markdown files are the source of the [Cactulator wiki](https://github.com/StochasticCactus/Cactulator/wiki).
Links between pages use wiki-style names (`[Graphing](Graphing)`), so they work on the wiki rather than when browsing this folder.

When a change to this folder lands on `main`, the `wiki` GitHub Actions workflow (`.github/workflows/wiki.yml`) copies every page to the wiki repository. Edit pages here, not in the wiki's web editor: web edits are overwritten on the next publish.

**One-time setup:** GitHub only creates a wiki's Git repository after its first page exists. Open the repository's **Wiki** tab, click **Create the first page**, save it with any content, then re-run the workflow (Actions → wiki → Run workflow). It will replace that page with `Home.md`.

To publish by hand instead:

```bash
git clone https://github.com/StochasticCactus/Cactulator.wiki.git /tmp/cactulator-wiki
```

```bash
rsync -a --delete --exclude .git --exclude README.md wiki/ /tmp/cactulator-wiki/
```

```bash
cd /tmp/cactulator-wiki && git add -A && git commit -m "Update wiki" && git push
```
