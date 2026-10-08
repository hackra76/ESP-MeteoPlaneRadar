import re

with open('MeteoPlaneRadar/WebPage.h', 'r', encoding='utf-8') as f:
    content = f.read()

html_keys = set(re.findall(r'data-i18n=["\']([^"\']+)["\']', content))
js_keys = set(re.findall(r'\bt\(["\']([^"\']+)["\']\)', content))
all_used = html_keys | js_keys
print('Total keys used in HTML/JS:', len(all_used))

# Let's find cs, sk, en sections
for lang in ['cs', 'sk', 'en']:
    m = re.search(rf'\b{lang}\s*:\s*\{{(.*?)\n\s*\}}', content, re.DOTALL)
    if m:
        keys = set(re.findall(r'([a-zA-Z0-9_]+)\s*:', m.group(1)))
        unused = keys - all_used
        print(f'{lang} dict keys: {len(keys)}, unused: {len(unused)}')
        if unused:
            print(f'  Unused in {lang}:', sorted(list(unused)))
