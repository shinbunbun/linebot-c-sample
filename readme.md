# linebot-c-sample

## 自己署名証明書作成

参考: <https://weblabo.oscasierra.net/openssl-gencert-1/>

```terminal
openssl genrsa 2048 > server.key
openssl req -new -key server.key > server.csr
openssl x509 -req -days 3650 -signkey server.key < server.csr > server.crt
```

## build

command+shift+b

## 実行

f5

## テスト

curl --insecure --verbose https://localhost:8765/