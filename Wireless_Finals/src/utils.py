# Impport libraries
import os, logging, boto3, traceback
from datetime import datetime, timezone
from email.mime.multipart import MIMEMultipart
from email.mime.text      import MIMEText
from email.mime.image     import MIMEImage
import smtplib

# Basic configuration
logging.basicConfig(level=logging.INFO,
                    format="%(asctime)s %(name)s %(levelname)s %(message)s")
log = logging.getLogger("utils")

# Grad environment variables saved to EC2 server
BUCKET  = os.getenv("BUCKET",  "wireless-network-photos")
REGION  = os.getenv("REGION",  "us-west-1")
SITEURL = f"http://{BUCKET}.s3-website.{REGION}.amazonaws.com"

# Supported image file extensions for listing
_IMG_EXT = {".jpg", ".jpeg", ".png", ".gif"}

# AWS services to be used
s3  = boto3.client("s3",  region_name=REGION)

# List all image objects in bucket from latest to last, generate HRML index page, and upload page to S3
def update_log_page(bucket: str = BUCKET) -> str:
    
    # Retrieve the list of images from S3
    resp = s3.list_objects_v2(Bucket=bucket)
    imgs = [o for o in resp.get("Contents", [])
            if os.path.splitext(o["Key"].lower())[1] in _IMG_EXT]

    # Sort images by LastModified
    imgs.sort(key=lambda o: o["LastModified"], reverse=True)

    #Build HTML table rows
    rows = []
    for o in imgs:
        # Get timestamp
        ts  = o["LastModified"].astimezone(timezone.utc).strftime("%Y‑%m‑%d %H:%M:%S UTC")
        # Construct the public URL for the image
        url = f"https://{bucket}.s3.amazonaws.com/{o['Key']}"
        # Append a table row with timestamp, filename link, and thumbnail
        rows.append(f"""
        <tr>
          <td>{ts}</td>
          <td><a href="{url}">{o['Key']}</a></td>
          <td><img src="{url}" style="max-width:160px"></td>
        </tr>""")

    # Wrap rows in a complete HTML document for the motion log page
    html = f"""<!doctype html><html><head>
<meta charset="utf-8"><title>Motion Log</title>
<style>body{{font-family:sans-serif}}table{{border-collapse:collapse}}
td{{padding:6px;border:1px solid #ccc}}</style></head><body>
<h1>Motion Log</h1>
<table><thead><tr><th>Timestamp (UTC)</th><th>File</th><th>Preview</th></tr></thead>
<tbody>{''.join(rows)}</tbody></table></body></html>"""

    # Upload the HTML index to S3 under log/index.html, ensuring correct MIME type
    s3.put_object(Bucket=bucket, Key="log/index.html", Body=html.encode(), ContentType="text/html")

    # Return the public URL for the generated log page
    return f"{SITEURL}/log/index.html"

# Send email helper
def send_email(to_addr: str, subject: str, plain: str, html: str, img_bytes: bytes) -> None:

    # Grab email credentials from environment variables
    sender = os.environ["GMAIL_USER"]
    pwd    = os.environ["GMAIL_APP_PASS"]

    # Attach a multipart/alternative part for plain text and HTM
    msg = MIMEMultipart("related")
    msg["From"], msg["To"], msg["Subject"] = sender, to_addr, subject

    # Attach the image data and reference it by Content-ID <snap>
    alt = MIMEMultipart("alternative")
    alt.attach(MIMEText(plain, "plain"))
    alt.attach(MIMEText(html,  "html"))
    msg.attach(alt)

    img = MIMEImage(img_bytes)
    img.add_header("Content-ID", "<snap>")
    msg.attach(img)

    # Connect to Gmail's SMTP server, secure the connection, and send
    with smtplib.SMTP("smtp.gmail.com", 587) as s:
        s.starttls()
        s.login(sender, pwd)
        s.sendmail(sender, [to_addr], msg.as_string())
