# utils.py  ────────────────────────────────────────────────────────────────
import os, logging, boto3, traceback
from datetime import datetime, timezone
from email.mime.multipart import MIMEMultipart
from email.mime.text      import MIMEText
from email.mime.image     import MIMEImage
import smtplib

logging.basicConfig(level=logging.INFO,
                    format="%(asctime)s %(name)s %(levelname)s %(message)s")
log = logging.getLogger("utils")

BUCKET  = os.getenv("BUCKET",  "wireless-network-photos")
REGION  = os.getenv("REGION",  "us-west-1")
SITEURL = f"http://{BUCKET}.s3-website.{REGION}.amazonaws.com"

_IMG_EXT = {".jpg", ".jpeg", ".png", ".gif"}

s3  = boto3.client("s3",  region_name=REGION)

# ─── new helper: rebuild (or create) the log/index.html page ──────────────
def update_log_page(bucket: str = BUCKET) -> str:
    """
    List all image objects, newest first, write log/index.html, return its URL.
    """

    # 1) pull object list
    resp = s3.list_objects_v2(Bucket=bucket)
    imgs = [o for o in resp.get("Contents", [])
            if os.path.splitext(o["Key"].lower())[1] in _IMG_EXT]

    # 2) sort newest → oldest
    imgs.sort(key=lambda o: o["LastModified"], reverse=True)

    # 3) build rows
    rows = []
    for o in imgs:
        ts  = o["LastModified"].astimezone(timezone.utc)\
                               .strftime("%Y‑%m‑%d %H:%M:%S UTC")
        url = f"https://{bucket}.s3.amazonaws.com/{o['Key']}"
        rows.append(f"""
        <tr>
          <td>{ts}</td>
          <td><a href="{url}">{o['Key']}</a></td>
          <td><img src="{url}" style="max-width:160px"></td>
        </tr>""")

    html = f"""<!doctype html><html><head>
<meta charset="utf-8"><title>Motion Log</title>
<style>body{{font-family:sans-serif}}table{{border-collapse:collapse}}
td{{padding:6px;border:1px solid #ccc}}</style></head><body>
<h1>Motion Log</h1>
<table><thead><tr><th>Timestamp (UTC)</th><th>File</th><th>Preview</th></tr></thead>
<tbody>{''.join(rows)}</tbody></table></body></html>"""

    s3.put_object(Bucket=bucket,
                  Key="log/index.html",
                  Body=html.encode(),
                  ContentType="text/html")          # so the page loads

    return f"{SITEURL}/log/index.html"

# ─── unchanged: e‑mail helper (still Gmail SMTP) ──────────────────────────
def send_email(to_addr: str,
               subject: str,
               plain: str,
               html: str,
               img_bytes: bytes) -> None:

    sender = os.environ["GMAIL_USER"]
    pwd    = os.environ["GMAIL_APP_PASS"]

    msg = MIMEMultipart("related")
    msg["From"], msg["To"], msg["Subject"] = sender, to_addr, subject

    alt = MIMEMultipart("alternative")
    alt.attach(MIMEText(plain, "plain"))
    alt.attach(MIMEText(html,  "html"))
    msg.attach(alt)

    img = MIMEImage(img_bytes)
    img.add_header("Content-ID", "<snap>")
    msg.attach(img)

    with smtplib.SMTP("smtp.gmail.com", 587) as s:
        s.starttls()
        s.login(sender, pwd)
        s.sendmail(sender, [to_addr], msg.as_string())