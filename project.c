// perpustakaan.c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define MAX_BOOKS 200
#define MAX_PINJAM 1000

// denda per hari setelah masa pinjam (7 hari)
#define DENDA_PER_HARI 1000
#define BATAS_PINJAM_HARI 7

typedef struct {
    int id;
    char judul[200];
    char pengarang[100];
    char lokasi[100];
    int stok;
} Buku;

typedef struct {
    char noKTM[50];
    char nama[100];
    int idBuku;
    char tanggalPinjam[20]; // YYYY-MM-DD
} Peminjaman;

/* --- util tanggal --- */
void tanggalSekarang(char out[20]) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(out, "%04d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
}

int hariSelisih(const char *tglMulai, const char *tglAkhir) {
    struct tm tm1 = {0}, tm2 = {0};
    int y1, m1, d1, y2, m2, d2;
    if (sscanf(tglMulai, "%d-%d-%d", &y1, &m1, &d1) != 3) return 0;
    if (sscanf(tglAkhir, "%d-%d-%d", &y2, &m2, &d2) != 3) return 0;
    tm1.tm_year = y1 - 1900; tm1.tm_mon = m1 - 1; tm1.tm_mday = d1;
    tm2.tm_year = y2 - 1900; tm2.tm_mon = m2 - 1; tm2.tm_mday = d2;
    time_t time1 = mktime(&tm1);
    time_t time2 = mktime(&tm2);
    if (time1 == (time_t)-1 || time2 == (time_t)-1) return 0;
    double diff = difftime(time2, time1);
    return (int)(diff / (60 * 60 * 24));
}

/* --- buku file handling --- */
int loadBooks(Buku books[], int maxBooks) {
    FILE *f = fopen("buku.txt", "r");
    if (!f) {
        // jika belum ada file, buat beberapa entri contoh
        books[0].id = 1;
        strcpy(books[0].judul, "Pemrograman C untuk Pemula");
        strcpy(books[0].pengarang, "Andi Programmer");
        strcpy(books[0].lokasi, "Rak A1");
        books[0].stok = 5;

        books[1].id = 2;
        strcpy(books[1].judul, "Struktur Data dan Algoritma");
        strcpy(books[1].pengarang, "Budi Data");
        strcpy(books[1].lokasi, "Rak B2");
        books[1].stok = 3;

        books[2].id = 3;
        strcpy(books[2].judul, "Jaringan Komputer");
        strcpy(books[2].pengarang, "Citra Net");
        strcpy(books[2].lokasi, "Rak C3");
        books[2].stok = 4;

        int n = 3;
        // simpan ke file supaya persist
        FILE *fw = fopen("buku.txt", "w");
        if (fw) {
            for (int i = 0; i < n; i++) {
                fprintf(fw, "%d|%s|%s|%s|%d\n",
                    books[i].id, books[i].judul, books[i].pengarang, books[i].lokasi, books[i].stok);
            }
            fclose(fw);
        }
        return n;
    }

    char line[1024];
    int count = 0;
    while (fgets(line, sizeof(line), f) && count < maxBooks) {
        // format: id|judul|pengarang|lokasi|stok
        char *p = strtok(line, "\n");
        if (!p) continue;
        int id, stok;
        char judul[200], pengarang[100], lokasi[100];
        // gunakan sscanf setelah mengganti '|' -> reading tokens
        char *tok;
        tok = strtok(line, "|");
        if (!tok) continue;
        id = atoi(tok);
        tok = strtok(NULL, "|");
        if (!tok) continue;
        strncpy(judul, tok, sizeof(judul));
        tok = strtok(NULL, "|");
        if (!tok) continue;
        strncpy(pengarang, tok, sizeof(pengarang));
        tok = strtok(NULL, "|");
        if (!tok) continue;
        strncpy(lokasi, tok, sizeof(lokasi));
        tok = strtok(NULL, "|");
        if (!tok) continue;
        stok = atoi(tok);

        books[count].id = id;
        strncpy(books[count].judul, judul, sizeof(books[count].judul));
        strncpy(books[count].pengarang, pengarang, sizeof(books[count].pengarang));
        strncpy(books[count].lokasi, lokasi, sizeof(books[count].lokasi));
        books[count].stok = stok;
        count++;
    }
    fclose(f);
    return count;
}

void saveBooks(Buku books[], int n) {
    FILE *f = fopen("buku.txt", "w");
    if (!f) {
        printf("Gagal menyimpan data buku!\n");
        return;
    }
    for (int i = 0; i < n; i++) {
        fprintf(f, "%d|%s|%s|%s|%d\n",
            books[i].id, books[i].judul, books[i].pengarang, books[i].lokasi, books[i].stok);
    }
    fclose(f);
}

void tampilkanBuku(Buku books[], int n) {
    printf("\n===== DAFTAR BUKU =====\n");
    printf("ID | Judul (Lokasi) - Pengarang - Stok\n");
    for (int i = 0; i < n; i++) {
        printf("%2d | %s (%s) - %s - Stok: %d\n",
            books[i].id, books[i].judul, books[i].lokasi, books[i].pengarang, books[i].stok);
    }
    printf("=======================\n");
}

int findBookIndexById(Buku books[], int n, int id) {
    for (int i = 0; i < n; i++) if (books[i].id == id) return i;
    return -1;
}

/* --- peminjaman file handling --- */
int loadPeminjaman(Peminjaman arr[], int max) {
    FILE *f = fopen("peminjaman.txt", "r");
    if (!f) return 0;
    char line[512];
    int count = 0;
    while (fgets(line, sizeof(line), f) && count < max) {
        // format: noKTM|nama|idBuku|tanggalPinjam
        char *tok = strtok(line, "\n");
        if (!tok) continue;
        char *p = strtok(line, "|");
        if (!p) continue;
        strncpy(arr[count].noKTM, p, sizeof(arr[count].noKTM));
        p = strtok(NULL, "|"); if (!p) continue;
        strncpy(arr[count].nama, p, sizeof(arr[count].nama));
        p = strtok(NULL, "|"); if (!p) continue;
        arr[count].idBuku = atoi(p);
        p = strtok(NULL, "|"); if (!p) continue;
        strncpy(arr[count].tanggalPinjam, p, sizeof(arr[count].tanggalPinjam));
        count++;
    }
    fclose(f);
    return count;
}

void savePeminjaman(Peminjaman arr[], int n) {
    FILE *f = fopen("peminjaman.txt", "w");
    if (!f) {
        printf("Gagal menyimpan data peminjaman!\n");
        return;
    }
    for (int i = 0; i < n; i++) {
        fprintf(f, "%s|%s|%d|%s\n", arr[i].noKTM, arr[i].nama, arr[i].idBuku, arr[i].tanggalPinjam);
    }
    fclose(f);
}

void appendStrukPinjam(Peminjaman p, Buku buku) {
    FILE *f = fopen("struk_peminjaman.txt", "a");
    if (!f) return;
    fprintf(f, "===== STRUK PEMINJAMAN =====\n");
    fprintf(f, "Nama Peminjam : %s\n", p.nama);
    fprintf(f, "No. KTM       : %s\n", p.noKTM);
    fprintf(f, "Judul Buku    : %s\n", buku.judul);
    fprintf(f, "Pengarang     : %s\n", buku.pengarang);
    fprintf(f, "Lokasi        : %s\n", buku.lokasi);
    fprintf(f, "Tanggal Pinjam: %s\n", p.tanggalPinjam);
    fprintf(f, "===========================\n\n");
    fclose(f);
}

void appendPengembalian(Peminjaman p, Buku buku, const char *tanggalKembali, int denda) {
    FILE *f = fopen("pengembalian.txt", "a");
    if (!f) return;
    fprintf(f, "===== NOTA PENGEMBALIAN =====\n");
    fprintf(f, "Nama Peminjam : %s\n", p.nama);
    fprintf(f, "No. KTM       : %s\n", p.noKTM);
    fprintf(f, "Judul Buku    : %s\n", buku.judul);
    fprintf(f, "Tanggal Pinjam: %s\n", p.tanggalPinjam);
    fprintf(f, "Tanggal Kembali: %s\n", tanggalKembali);
    fprintf(f, "Denda         : Rp%d\n", denda);
    fprintf(f, "=============================\n\n");
    fclose(f);
}

/* --- akun & login --- */
void registerAkun() {
    char username[100], password[100];
    FILE *f = fopen("akun.txt", "a");
    if (!f) {
        printf("Gagal membuka file akun!\n");
        return;
    }
    printf("\n===== REGISTER AKUN =====\n");
    printf("Masukkan username: ");
    scanf("%99s", username);
    printf("Masukkan password: ");
    scanf("%99s", password);

    fprintf(f, "%s %s\n", username, password);
    fclose(f);
    printf("Akun berhasil dibuat. Silakan login.\n");
}

int login(char loggedUser[100]) {
    char user[100], pass[100];
    printf("\n===== LOGIN =====\n");
    printf("Username: ");
    scanf("%99s", user);
    printf("Password: ");
    scanf("%99s", pass);

    FILE *f = fopen("akun.txt", "r");
    if (!f) {
        printf("Belum ada akun terdaftar. Silakan register dulu.\n");
        return 0;
    }
    char u[100], p[100];
    int ok = 0;
    while (fscanf(f, "%99s %99s", u, p) != EOF) {
        if (strcmp(u, user) == 0 && strcmp(p, pass) == 0) {
            ok = 1;
            break;
        }
    }
    fclose(f);
    if (ok) {
        strncpy(loggedUser, user, 100);
        // simpan riwayat login
        FILE *r = fopen("riwayat_login.txt", "a");
        if (r) {
            char tgl[30];
            time_t tt = time(NULL);
            struct tm tm = *localtime(&tt);
            sprintf(tgl, "%04d-%02d-%02d %02d:%02d:%02d",
                    tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                    tm.tm_hour, tm.tm_min, tm.tm_sec);
            fprintf(r, "%s | %s\n", user, tgl);
            fclose(r);
        }
        printf("Login berhasil. Selamat datang, %s!\n", user);
        return 1;
    } else {
        printf("Login gagal. Username atau password salah.\n");
        return 0;
    }
}

/* --- operasi pinjam & kembali --- */
void menuPinjam(Buku books[], int nBooks) {
    Peminjaman pem;
    char temp[200];
    printf("\n=== PINJAM BUKU ===\n");
    printf("Masukkan No. KTM: ");
    scanf("%49s", pem.noKTM);
    getchar(); // buang newline
    printf("Masukkan Nama   : ");
    fgets(pem.nama, sizeof(pem.nama), stdin);
    pem.nama[strcspn(pem.nama, "\n")] = 0;

    tampilkanBuku(books, nBooks);
    printf("Masukkan ID buku yang ingin dipinjam: ");
    if (scanf("%d", &pem.idBuku) != 1) {
        printf("Input tidak valid.\n");
        return;
    }
    int idx = findBookIndexById(books, nBooks, pem.idBuku);
    if (idx == -1) {
        printf("ID buku tidak ditemukan.\n");
        return;
    }
    if (books[idx].stok <= 0) {
        printf("Maaf, stok buku habis.\n");
        return;
    }

    tanggalSekarang(pem.tanggalPinjam);

    // catat di peminjaman.txt (append)
    FILE *f = fopen("peminjaman.txt", "a");
    if (!f) {
        printf("Gagal mencatat peminjaman.\n");
        return;
    }
    fprintf(f, "%s|%s|%d|%s\n", pem.noKTM, pem.nama, pem.idBuku, pem.tanggalPinjam);
    fclose(f);

    // kurangi stok dan simpan buku
    books[idx].stok--;
    saveBooks(books, nBooks);

    // buat struk peminjaman
    appendStrukPinjam(pem, books[idx]);

    printf("Peminjaman berhasil. Struk telah disimpan.\n");
}

void menuKembalikan(Buku books[], int nBooks) {
    char noKTM[50];
    int idBuku;
    printf("\n=== PENGEMBALIAN BUKU ===\n");
    printf("Masukkan No. KTM: ");
    scanf("%49s", noKTM);
    printf("Masukkan ID buku yang dikembalikan: ");
    if (scanf("%d", &idBuku) != 1) {
        printf("Input tidak valid.\n");
        return;
    }

    // load semua peminjaman
    Peminjaman arr[MAX_PINJAM];
    int n = loadPeminjaman(arr, MAX_PINJAM);
    int found = -1;
    for (int i = 0; i < n; i++) {
        if (arr[i].idBuku == idBuku && strcmp(arr[i].noKTM, noKTM) == 0) {
            found = i;
            break;
        }
    }
    if (found == -1) {
        printf("Data peminjaman tidak ditemukan (cek No. KTM dan ID buku).\n");
        return;
    }
    // hitung denda
    char tglKembali[20];
    tanggalSekarang(tglKembali);
    int hari = hariSelisih(arr[found].tanggalPinjam, tglKembali);
    int denda = 0;
    if (hari > BATAS_PINJAM_HARI) {
        denda = (hari - BATAS_PINJAM_HARI) * DENDA_PER_HARI;
    }

    // update stok buku
    int idxB = findBookIndexById(books, nBooks, idBuku);
    if (idxB != -1) {
        books[idxB].stok++;
        saveBooks(books, nBooks);
    }

    // catat pengembalian
    appendPengembalian(arr[found], (idxB!=-1?books[idxB]:(Buku){idBuku,"(judul tidak ada)","-","-",0}), tglKembali, denda);

    // hapus entri peminjaman dari array dan simpan ulang
    for (int i = found; i < n - 1; i++) arr[i] = arr[i + 1];
    savePeminjaman(arr, n - 1);

    printf("Pengembalian berhasil.\n");
    printf("Hari pinjam : %d hari\n", hari);
    printf("Denda       : Rp%d\n", denda);
}

/* --- main --- */
int main() {
    Buku books[MAX_BOOKS];
    int nBooks = loadBooks(books, MAX_BOOKS);

    printf("=====================================\n");
    printf("       SISTEM PERPUSTAKAAN PINTAR    \n");
    printf("=====================================\n");
    printf("1. Register Akun\n");
    printf("2. Login\n");
    printf("Pilih: ");
    int pilihan;
    if (scanf("%d", &pilihan) != 1) return 0;
    if (pilihan == 1) {
        registerAkun();
    }

    char user[100];
    if (!login(user)) {
        return 0;
    }

    int running = 1;
    while (running) {
        printf("\n===== HALAMAN UTAMA =====\n");
        printf("1. Pinjam Buku\n");
        printf("2. Kembalikan Buku\n");
        printf("3. Lihat Daftar Buku\n");
        printf("4. Logout / Keluar\n");
        printf("Pilih: ");
        int m;
        if (scanf("%d", &m) != 1) break;
        getchar(); // buang newline
        switch (m) {
            case 1:
                menuPinjam(books, nBooks);
                break;
            case 2:
                menuKembalikan(books, nBooks);
                break;
            case 3:
                tampilkanBuku(books, nBooks);
                break;
            case 4:
                running = 0;
                printf("Logout. Sampai jumpa %s!\n", user);
                break;
            default:
                printf("Pilihan tidak valid.\n");
        }
    }

    return 0;
}
