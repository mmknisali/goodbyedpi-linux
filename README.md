# GoodbyeDPI Linux

[![Version](https://img.shields.io/badge/version-0.2.3-blue.svg)](https://github.com/mmknisali/goodbyedpi-linux)
[![License](https://img.shields.io/badge/license-Apache--2.0-green.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Linux-lightgrey.svg)](https://github.com/mmknisali/goodbyedpi-linux)
[![Docker](https://img.shields.io/badge/docker-ready-blue.svg)](Dockerfile)

**Linux sistemler için geliştirilmiş, Deep Packet Inspection (DPI) davranışlarını analiz etmeye ve ağ trafiği üzerinde çeşitli paket işleme tekniklerini test etmeye yönelik genel amaçlı bir yardımcı araçtır.**

GoodbyeDPI Linux, ağ trafiğinin kullanıcı tarafında nasıl işlendiğini incelemek, DPI sistemlerinin davranışlarını analiz etmek ve farklı paket manipülasyon tekniklerini denemek amacıyla geliştirilmiştir. Yazılım herhangi bir içerik barındırmaz, sunmaz veya dağıtmaz.

---

## 🌟 Özellikler

- **🔍 DPI Davranış Analizi** – DPI sistemlerinin paketleri nasıl değerlendirdiğini test etmeye yönelik teknikler
- **🚀 Çoklu Paket İşleme Teknikleri** – Fragmentation, header düzenleme, TTL ayarlamaları, sahte paketler
- **🐳 Docker Desteği** – Sistemi etkilemeden izole ortamda çalıştırma
- **⚙️ Esnek Yapılandırma** – Komut satırı, yapılandırma dosyası ve legacy modlar
- **🔄 systemd Entegrasyonu** – Servis olarak çalıştırma ve otomatik yeniden başlatma
- **📊 Gerçek Zamanlı İstatistikler** – Paket işleme ve modifikasyon verileri
- **🛡️ Güvenlik Odaklı** – Thread-safe yapı, ayrıcalık kontrollü çalışma

---

## 📋 İçindekiler

- [Hızlı Başlangıç (Docker)](#-hızlı-başlangıç-docker---önerilen)
- [Hızlı Başlangıç (Yerel Kurulum)](#-hızlı-başlangıç-yerel-kurulum)
- [Nasıl Çalışır](#-nasıl-çalışır)
- [Kurulum](#-kurulum)
- [Kullanım](#-kullanım)
- [Yapılandırma](#️-yapılandırma)
- [Legacy Modlar](#-legacy-modlar)
- [Sorun Giderme](#-sorun-giderme)
- [Performans](#-performans)
- [Katkı](#-katkı)
- [Lisans](#-lisans)

---

## 🐳 Hızlı Başlangıç (Docker - Önerilen)

**Sisteminize doğrudan kurulum yapmadan, izole bir konteyner içinde çalıştırın.**

### Gereksinimler
- Docker kurulu olmalı ([Docker Kurulumu](https://docs.docker.com/get-docker/))
- Linux ana sistem (veya Windows için WSL2)

### Tek Komutla Başlatma

```bash
chmod +x docker-start.sh
./docker-start.sh
```

### Manuel Docker Kullanımı

```bash
docker-compose up -d
docker-compose logs -f
docker-compose down
```

### Make ile Kullanım

```bash
make build
make up
make logs
make down
make help
```

**Detaylar için [DOCKER_GUIDE.md](DOCKER_GUIDE.md) dosyasına bakın.**

---

## 💻 Hızlı Başlangıç (Yerel Kurulum)

**Maksimum performans için doğrudan sistem üzerinde çalıştırın.**

### Ubuntu/Debian

```bash
sudo apt update
sudo apt install build-essential cmake libnetfilter-queue-dev libmnl-dev iptables
chmod +x scripts/install.sh
sudo scripts/install.sh
sudo goodbyedpi -9
```

### Fedora/RHEL/CentOS

```bash
sudo dnf install gcc cmake libnetfilter_queue-devel libmnl-devel iptables
chmod +x scripts/install.sh
sudo scripts/install.sh
sudo goodbyedpi -9
```

### Arch Linux

```bash
sudo pacman -S base-devel cmake libnetfilter_queue libmnl iptables
chmod +x scripts/install.sh
sudo scripts/install.sh
sudo goodbyedpi -9
```

### Manuel Derleme

```bash
git clone https://github.com/mmknisali/goodbyedpi-linux.git
cd goodbyedpi-linux
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
sudo goodbyedpi -9
```

---

## 🔍 Nasıl Çalışır

GoodbyeDPI, Linux netfilter (NFQUEUE) altyapısını kullanarak ağ paketlerini kullanıcı alanında yakalar ve işler. Amaç, DPI sistemlerinin paketleri nasıl analiz ettiğini test etmek ve farklı işleme tekniklerinin etkilerini gözlemlemektir.

Kullanılan başlıca teknikler:
1. TCP Fragmentation  
2. Header düzenlemeleri  
3. TTL ayarlamaları  
4. Sahte paket enjeksiyonu  
5. Sequence / checksum varyasyonları  
6. QUIC trafiği kontrolü  

---

## 🚀 Kullanım

```bash
sudo goodbyedpi -9
sudo goodbyedpi -9 -d
sudo goodbyedpi -9 -v
sudo goodbyedpi -c /etc/goodbyedpi/goodbyedpi.conf
```

---

## ⚙️ Yapılandırma

```ini
[general]
daemon = true
verbose = true

[legacy_modes]
legacy_mode = 9
```

---

## ⚠️ Yasal Uyarı

Bu yazılım, ağ trafiği analizi, DPI (Deep Packet Inspection) davranışlarının incelenmesi ve genel amaçlı ağ testleri kapsamında geliştirilmiş **genel amaçlı bir yazılımdır**.

Bu proje:
- Herhangi bir içerik barındırmaz, sunmaz veya dağıtmaz.
- Belirli bir web sitesi, servis, platform veya ülkeye özel olarak tasarlanmamıştır.
- Kullanıcıya doğrudan içerik erişimi sağlamaz.

Yazılımın nasıl ve hangi amaçla kullanılacağı tamamen kullanıcının sorumluluğundadır. Kullanıcılar, yazılımı kullanırken yürürlükteki tüm yerel ve ulusal mevzuata, **Türkiye Cumhuriyeti 5651 sayılı Kanun** dahil olmak üzere, uymakla yükümlüdür.

Geliştirici ve katkıda bulunanlar, yazılımın kullanımından doğabilecek hukuki veya cezai sonuçlardan sorumlu tutulamaz.

---

## 📜 Lisans

Bu proje **Apache License 2.0** ile lisanslanmıştır. Detaylar için [LICENSE](LICENSE) dosyasına bakınız.

---

<div align="center">

**[⬆ Başa Dön](#goodbyedpi-linux)**

Açık kaynak topluluğu için geliştirildi.

</div>