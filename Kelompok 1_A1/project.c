
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#ifdef _WIN32
  #include <windows.h>
  #define STRCASECMP _stricmp
#else
  #include <unistd.h>
  #define STRCASECMP strcasecmp
#endif

#define MAX_BUKU 200
#define MAX_AKUN 200
#define MAX_PEMINJAMAN 500

#ifdef _WIN32
  #define CLEAR_CMD "cls"
#else
  #define CLEAR_CMD "clear"
#endif

/* ------------------ Prototypes (urut sesuai flow) ------------------ */
/* Utility */
void sleep_ms(int ms);
void clear_screen(void);
void loading(const char *msg);
void trim_newline(char *s);
int contains_ci(const char *hay, const char *needle);

/* Data & helpers tanggal */
void today_str(char buf[16]);
time_t parse_date(const char *s);
void add_days_to_date(const char *src, int days, char out[16]);
int days_between(const char *a, const char *b);

/* Persistence */
void load_books(const char *filename);
void save_books(const char *filename);
void load_accounts(const char *filename);
void save_accounts(const char *filename);
void load_peminjaman(const char *filename);
void save_peminjaman(const char *filename);

/* Akun (anggota) */
int akunExists(const char *username);
void registerAkunUser(int isAdminDefault);
int loginUser(char outUsername[32], int *outIsAdmin);

/* Buku */
struct Buku* findBukuById(int id);
void tambahBuku(void);
void editBuku(void);
void hapusBuku(void);
void tampilkanDaftarBuku(void);
void laporanStok(void);
void cariBukuMenu(void);
void sortBukuByTitle(void);

/* Peminjaman */
void pinjamBuku(const char *username);
void kembalikanBuku(const char *username);
void lihatRiwayatUser(const char *username);

/* UI */
void adminMenu(const char *username);
void userMenu(const char *username);

/* Utility: seed */
void seed_data_if_needed(void);

/* ------------------ Struktur Data ------------------ */
struct Buku {
    int id;
    char judul[128];
    char pengarang[64];
    int stok;
    int total; // total pernah dimiliki
};

struct Akun {
    char username[32];
    char password[32];
    int isAdmin; // 1 = admin, 0 = user
};

struct Peminjaman {
    int id; // id peminjaman
    char username[32];
    int idBuku;
    char judul[128];
    char tanggalPinjam[16]; // dd-mm-YYYY
    char tanggalJatuhTempo[16];
    char tanggalKembali[16]; // kosong jika belum kembali
    double denda;
    int returned; // 0 belum, 1 sudah
};

/* ------------------ Global state ------------------ */
static struct Buku daftarBuku[MAX_BUKU];
static int jumlahBuku = 0;

static struct Akun daftarAkun[MAX_AKUN];
static int jumlahAkun = 0;

static struct Peminjaman daftarPinjam[MAX_PEMINJAMAN];
static int jumlahPinjam = 0;

static int nextBukuId = 1;
static int nextPinjamId = 1;

/* ------------------ Utility implementations ------------------ */
void sleep_ms(int ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

void clear_screen(void) { system(CLEAR_CMD); }

void loading(const char *msg) {
    if (!msg) msg = "Processing";
    printf("%s", msg); fflush(stdout);
    for (int i = 0; i < 6; ++i) { printf("."); fflush(stdout); sleep_ms(80); }
    printf("\n");
}

void trim_newline(char *s) {
    if (!s) return;
    size_t len = strlen(s);
    while (len > 0 && (s[len-1] == '\n' || s[len-1] == '\r')) { s[len-1] = '\0'; len--; }
}

int contains_ci(const char *hay, const char *needle) {
    if (!hay || !needle) return 0;
    size_t h = strlen(hay), n = strlen(needle);
    if (n == 0) return 1;
    for (size_t i = 0; i + n <= h; ++i) {
        size_t j = 0;
        for (; j < n; ++j) {
            if (tolower((unsigned char)hay[i+j]) != tolower((unsigned char)needle[j])) break;
        }
        if (j == n) return 1;
    }
    return 0;
}

/* ------------------ Tanggal helper ------------------ */
void today_str(char buf[16]) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(buf, "%02d-%02d-%04d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
}

time_t parse_date(const char *s) {
    int d, m, y;
    if (sscanf(s, "%d-%d-%d", &d, &m, &y) != 3) return (time_t)-1;
    struct tm tm = {0};
    tm.tm_mday = d; tm.tm_mon = m - 1; tm.tm_year = y - 1900;
    tm.tm_hour = 0; tm.tm_min = 0; tm.tm_sec = 0;
    return mktime(&tm);
}

void add_days_to_date(const char *src, int days, char out[16]) {
    time_t t = parse_date(src);
    if (t == (time_t)-1) { strcpy(out, "--"); return; }
    t += (time_t)days * 24 * 3600;
    struct tm tm = *localtime(&t);
    sprintf(out, "%02d-%02d-%04d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
}

int days_between(const char *a, const char *b) {
    time_t ta = parse_date(a); time_t tb = parse_date(b);
    if (ta == (time_t)-1 || tb == (time_t)-1) return 0;
    double diff = difftime(tb, ta);
    return (int)(diff / (24*3600));
}

/* ------------------ Persistence (file I/O) ------------------ */
void load_books(const char *filename) {
    FILE *f = fopen(filename, "r"); if (!f) return;
    jumlahBuku = 0; nextBukuId = 1; char line[512];
    while (fgets(line, sizeof(line), f) && jumlahBuku < MAX_BUKU) {
        trim_newline(line); if (strlen(line) == 0) continue;
        struct Buku b = {0}; char *tok = strtok(line, "|"); if (!tok) continue; b.id = atoi(tok);
        tok = strtok(NULL, "|"); if (!tok) continue; strncpy(b.judul, tok, sizeof(b.judul));
        tok = strtok(NULL, "|"); if (!tok) continue; strncpy(b.pengarang, tok, sizeof(b.pengarang));
        tok = strtok(NULL, "|"); if (!tok) continue; b.stok = atoi(tok);
        tok = strtok(NULL, "|"); if (tok) b.total = atoi(tok); else b.total = b.stok;
        daftarBuku[jumlahBuku++] = b; if (b.id >= nextBukuId) nextBukuId = b.id + 1;
    }
    fclose(f);
}

void save_books(const char *filename) {
    FILE *f = fopen(filename, "w"); if (!f) return;
    for (int i = 0; i < jumlahBuku; ++i) {
        struct Buku *b = &daftarBuku[i]; fprintf(f, "%d|%s|%s|%d|%d\n", b->id, b->judul, b->pengarang, b->stok, b->total);
    }
    fclose(f);
}

void load_accounts(const char *filename) {
    FILE *f = fopen(filename, "r"); if (!f) return;
    jumlahAkun = 0; char line[256];
    while (fgets(line, sizeof(line), f) && jumlahAkun < MAX_AKUN) {
        trim_newline(line); if (strlen(line) == 0) continue;
        struct Akun a = {0}; char *tok = strtok(line, "|"); if (!tok) continue; strncpy(a.username, tok, sizeof(a.username));
        tok = strtok(NULL, "|"); if (!tok) continue; strncpy(a.password, tok, sizeof(a.password));
        tok = strtok(NULL, "|"); a.isAdmin = tok ? atoi(tok) : 0; daftarAkun[jumlahAkun++] = a;
    }
    fclose(f);
}

void save_accounts(const char *filename) {
    FILE *f = fopen(filename, "w"); if (!f) return;
    for (int i = 0; i < jumlahAkun; ++i) fprintf(f, "%s|%s|%d\n", daftarAkun[i].username, daftarAkun[i].password, daftarAkun[i].isAdmin);
    fclose(f);
}

void load_peminjaman(const char *filename) {
    FILE *f = fopen(filename, "r"); if (!f) return;
    jumlahPinjam = 0; nextPinjamId = 1; char line[512];
    while (fgets(line, sizeof(line), f) && jumlahPinjam < MAX_PEMINJAMAN) {
        trim_newline(line); if (strlen(line) == 0) continue;
        struct Peminjaman p; memset(&p, 0, sizeof(p)); char *tok = strtok(line, "|"); if (!tok) continue; p.id = atoi(tok);
        tok = strtok(NULL, "|"); if (!tok) continue; strncpy(p.username, tok, sizeof(p.username));
        tok = strtok(NULL, "|"); if (!tok) continue; p.idBuku = atoi(tok);
        tok = strtok(NULL, "|"); if (!tok) continue; strncpy(p.judul, tok, sizeof(p.judul));
        tok = strtok(NULL, "|"); if (!tok) continue; strncpy(p.tanggalPinjam, tok, sizeof(p.tanggalPinjam));
        tok = strtok(NULL, "|"); if (!tok) continue; strncpy(p.tanggalJatuhTempo, tok, sizeof(p.tanggalJatuhTempo));
        tok = strtok(NULL, "|"); if (!tok) continue; strncpy(p.tanggalKembali, tok, sizeof(p.tanggalKembali));
        tok = strtok(NULL, "|"); if (!tok) continue; p.denda = atof(tok);
        tok = strtok(NULL, "|"); p.returned = tok ? atoi(tok) : 0;
        daftarPinjam[jumlahPinjam++] = p; if (p.id >= nextPinjamId) nextPinjamId = p.id + 1;
    }
    fclose(f);
}

void save_peminjaman(const char *filename) {
    FILE *f = fopen(filename, "w"); if (!f) return;
    for (int i = 0; i < jumlahPinjam; ++i) {
        struct Peminjaman *p = &daftarPinjam[i];
        fprintf(f, "%d|%s|%d|%s|%s|%s|%s|%.2f|%d\n", p->id, p->username, p->idBuku, p->judul, p->tanggalPinjam, p->tanggalJatuhTempo, p->tanggalKembali, p->denda, p->returned);
    }
    fclose(f);
}

/* ------------------ Akun management (anggota) ------------------ */
int akunExists(const char *username) {
    for (int i = 0; i < jumlahAkun; ++i) if (strcmp(daftarAkun[i].username, username) == 0) return 1;
    return 0;
}

void registerAkunUser(int isAdminDefault) {
    if (jumlahAkun >= MAX_AKUN) { printf("Kapasitas akun penuh\n"); return; }
    struct Akun a; printf("Masukkan username: "); scanf("%31s", a.username);
    if (akunExists(a.username)) { printf("Username sudah ada!\n"); return; }
    printf("Masukkan password: "); scanf("%31s", a.password);
    a.isAdmin = isAdminDefault; daftarAkun[jumlahAkun++] = a; save_accounts("akun.txt"); printf("Akun berhasil terdaftar.\n");
}

int loginUser(char outUsername[32], int *outIsAdmin) {
    char u[32], p[32]; printf("Username: "); scanf("%31s", u); printf("Password: "); scanf("%31s", p);
    for (int i = 0; i < jumlahAkun; ++i) {
        if (strcmp(daftarAkun[i].username, u) == 0 && strcmp(daftarAkun[i].password, p) == 0) { strcpy(outUsername, u); *outIsAdmin = daftarAkun[i].isAdmin; return 1; }
    }
    return 0;
}

/* ------------------ Buku operations ------------------ */
struct Buku* findBukuById(int id) { for (int i = 0; i < jumlahBuku; ++i) if (daftarBuku[i].id == id) return &daftarBuku[i]; return NULL; }

void tambahBuku(void) {
    if (jumlahBuku >= MAX_BUKU) { printf("Kapasitas buku penuh!\n"); return; }
    struct Buku b; b.id = nextBukuId++;
    printf("Masukkan judul buku: "); getchar(); fgets(b.judul, sizeof(b.judul), stdin); trim_newline(b.judul);
    printf("Masukkan pengarang: "); fgets(b.pengarang, sizeof(b.pengarang), stdin); trim_newline(b.pengarang);
    printf("Masukkan jumlah stok: "); if (scanf("%d", &b.stok) != 1) { printf("Input stok tidak valid\n"); return; }
    b.total = b.stok; daftarBuku[jumlahBuku++] = b; save_books("buku.txt"); printf("Buku berhasil ditambahkan (ID %d)\n", b.id);
}

void editBuku(void) {
    int id; printf("Masukkan ID buku yang ingin diedit: "); if (scanf("%d", &id) != 1) { printf("Input tidak valid\n"); return; }
    struct Buku *b = findBukuById(id); if (!b) { printf("Buku dengan ID %d tidak ditemukan\n", id); return; }
    printf("Edit buku (tekan enter untuk melewati)\n"); char buffer[256]; getchar();
    printf("Judul saat ini: %s\nMasukkan judul baru: ", b->judul); fgets(buffer, sizeof(buffer), stdin); trim_newline(buffer); if (strlen(buffer)>0) strncpy(b->judul, buffer, sizeof(b->judul));
    printf("Pengarang saat ini: %s\nMasukkan pengarang baru: ", b->pengarang); fgets(buffer, sizeof(buffer), stdin); trim_newline(buffer); if (strlen(buffer)>0) strncpy(b->pengarang, buffer, sizeof(b->pengarang));
    printf("Stok saat ini: %d\nMasukkan stok tambahan (angka, dapat negatif, kosong -> 0): ", b->stok); char stokbuf[32]; fgets(stokbuf, sizeof(stokbuf), stdin); trim_newline(stokbuf);
    if (strlen(stokbuf)>0) { int delta = atoi(stokbuf); b->stok += delta; if (b->stok < 0) b->stok = 0; }
    save_books("buku.txt"); printf("Buku berhasil diperbarui.\n");
}

void hapusBuku(void) {
    int id; printf("Masukkan ID buku yang ingin dihapus: "); if (scanf("%d", &id) != 1) { printf("Input tidak valid\n"); return; }
    int idx = -1; for (int i = 0; i < jumlahBuku; ++i) if (daftarBuku[i].id == id) { idx = i; break; }
    if (idx == -1) { printf("Buku tidak ditemukan\n"); return; }
    for (int i = idx; i < jumlahBuku - 1; ++i) daftarBuku[i] = daftarBuku[i+1]; jumlahBuku--; save_books("buku.txt"); printf("Buku dihapus.\n");
}

void tampilkanDaftarBuku(void) {
    if (jumlahBuku == 0) { printf("Belum ada buku.\n"); return; }
    sortBukuByTitle();
    printf("\n== Daftar Buku ==\n");
    for (int i = 0; i < jumlahBuku; ++i) printf("%d. %s | %s | Stok: %d\n", daftarBuku[i].id, daftarBuku[i].judul, daftarBuku[i].pengarang, daftarBuku[i].stok);
}

void laporanStok(void) {
    printf("\n=== Laporan Stok Buku ===\n");
    for (int i = 0; i < jumlahBuku; ++i) {
        printf("ID %d | %s | %s | Stok: %d | Total: %d\n", daftarBuku[i].id, daftarBuku[i].judul, daftarBuku[i].pengarang, daftarBuku[i].stok, daftarBuku[i].total);
    }
}

void cariBukuMenu(void) {
    printf("Masukkan kata kunci (judul atau pengarang): "); char kw[128]; getchar(); fgets(kw, sizeof(kw), stdin); trim_newline(kw);
    printf("Hasil pencarian untuk '%s':\n", kw);
    int found = 0; for (int i = 0; i < jumlahBuku; ++i) {
        if (contains_ci(daftarBuku[i].judul, kw) || contains_ci(daftarBuku[i].pengarang, kw)) {
            printf("ID %d | %s | %s | Stok: %d\n", daftarBuku[i].id, daftarBuku[i].judul, daftarBuku[i].pengarang, daftarBuku[i].stok); found = 1;
        }
    }
    if (!found) printf("Tidak ditemukan.\n");
}

void sortBukuByTitle(void) {
    for (int i = 0; i < jumlahBuku - 1; ++i) for (int j = i+1; j < jumlahBuku; ++j) {
        if (STRCASECMP(daftarBuku[i].judul, daftarBuku[j].judul) > 0) { struct Buku tmp = daftarBuku[i]; daftarBuku[i] = daftarBuku[j]; daftarBuku[j] = tmp; }
    }
}

/* ------------------ Peminjaman / Pengembalian ------------------ */
void pinjamBuku(const char *username) {
    tampilkanDaftarBuku();
    int id; printf("Masukkan ID buku untuk dipinjam (0 batal): "); if (scanf("%d", &id) != 1) { printf("Input tidak valid\n"); return; }
    if (id == 0) return; struct Buku *b = findBukuById(id); if (!b) { printf("ID tidak ditemukan\n"); return; }
    if (b->stok <= 0) { printf("Stok habis\n"); return; }
    struct Peminjaman p; p.id = nextPinjamId++; strncpy(p.username, username, sizeof(p.username)); p.idBuku = b->id; strncpy(p.judul, b->judul, sizeof(p.judul));
    today_str(p.tanggalPinjam); add_days_to_date(p.tanggalPinjam, 7, p.tanggalJatuhTempo); strcpy(p.tanggalKembali, "-"); p.denda = 0.0; p.returned = 0;
    daftarPinjam[jumlahPinjam++] = p; b->stok--; save_books("buku.txt"); save_peminjaman("pinjaman.txt");
    printf("Berhasil meminjam '%s'. Jatuh tempo: %s\n", p.judul, p.tanggalJatuhTempo);
}

void kembalikanBuku(const char *username) {
    printf("\nRiwayat peminjaman Anda (belum dikembalikan):\n"); int found = 0;
    for (int i = 0; i < jumlahPinjam; ++i) if (strcmp(daftarPinjam[i].username, username) == 0 && daftarPinjam[i].returned == 0) { printf("ID Pinjam %d | Buku ID %d | %s | Pinjam: %s | Jatuh Tempo: %s\n", daftarPinjam[i].id, daftarPinjam[i].idBuku, daftarPinjam[i].judul, daftarPinjam[i].tanggalPinjam, daftarPinjam[i].tanggalJatuhTempo); found = 1; }
    if (!found) { printf("Tidak ada peminjaman aktif.\n"); return; }
    int id; printf("Masukkan ID Pinjam yang ingin dikembalikan (0 batal): "); if (scanf("%d", &id) != 1) { printf("Input tidak valid\n"); return; }
    if (id == 0) return; struct Peminjaman *p = NULL; for (int i = 0; i < jumlahPinjam; ++i) if (daftarPinjam[i].id == id && strcmp(daftarPinjam[i].username, username) == 0) p = &daftarPinjam[i];
    if (!p) { printf("ID peminjaman tidak ditemukan atau bukan milik Anda.\n"); return; } if (p->returned) { printf("Sudah dikembalikan sebelumnya.\n"); return; }
    char today[16]; today_str(today); strcpy(p->tanggalKembali, today); int terlambat = days_between(p->tanggalJatuhTempo, today); double denda_per_hari = 2000.0; if (terlambat > 0) p->denda = terlambat * denda_per_hari; else p->denda = 0; p->returned = 1;
    struct Buku *b = findBukuById(p->idBuku); if (b) { b->stok++; }
    save_books("buku.txt"); save_peminjaman("pinjaman.txt"); printf("Buku '%s' dikembalikan. Denda: Rp %.2f\n", p->judul, p->denda);
}

void lihatRiwayatUser(const char *username) {
    printf("\nRiwayat peminjaman %s:\n", username); int found = 0;
    for (int i = 0; i < jumlahPinjam; ++i) if (strcmp(daftarPinjam[i].username, username) == 0) { struct Peminjaman *p = &daftarPinjam[i]; printf("ID %d | %s | Pinjam: %s | Jatuh Tempo: %s | Kembali: %s | Denda: %.2f | Status: %s\n", p->id, p->judul, p->tanggalPinjam, p->tanggalJatuhTempo, p->tanggalKembali, p->denda, p->returned?"Kembali":"Belum"); found = 1; }
    if (!found) printf("Belum ada riwayat.\n");
}

/* ------------------ UI menus (flow) ------------------ */
void adminMenu(const char *username) {
    int pilih = 0; while (1) {
        printf("\n== Admin Menu (%s) ==\n", username);
        printf("1. Lihat daftar buku\n2. Tambah buku\n3. Edit buku\n4. Hapus buku\n5. Laporan stok\n6. Lihat semua peminjaman\n7. Logout\nPilihan: ");
        if (scanf("%d", &pilih) != 1) { printf("Input invalid\n"); break; }
        clear_screen(); if (pilih == 1) tampilkanDaftarBuku(); else if (pilih == 2) tambahBuku(); else if (pilih == 3) editBuku(); else if (pilih == 4) hapusBuku(); else if (pilih == 5) laporanStok();
        else if (pilih == 6) { printf("\n== Semua Peminjaman ==\n"); for (int i = 0; i < jumlahPinjam; ++i) { struct Peminjaman *p = &daftarPinjam[i]; printf("ID %d | User %s | %s | Pinjam: %s | Jatuh Tempo: %s | Kembali: %s | Denda: %.2f | %s\n", p->id, p->username, p->judul, p->tanggalPinjam, p->tanggalJatuhTempo, p->tanggalKembali, p->denda, p->returned?"Kembali":"Belum"); } }
        else if (pilih == 7) break; else printf("Pilihan tidak dikenal\n");
    }
}

void userMenu(const char *username) {
    int pilih; while (1) {
        printf("\n== Menu User (%s) ==\n", username);
        printf("1. Lihat daftar buku\n2. Cari buku\n3. Pinjam buku\n4. Kembalikan buku\n5. Lihat riwayat\n6. Logout\nPilihan: ");
        if (scanf("%d", &pilih) != 1) { printf("Input invalid\n"); break; }
        clear_screen(); if (pilih == 1) tampilkanDaftarBuku(); else if (pilih == 2) cariBukuMenu(); else if (pilih == 3) pinjamBuku(username); else if (pilih == 4) kembalikanBuku(username);
        else if (pilih == 5) lihatRiwayatUser(username); else if (pilih == 6) break; else printf("Pilihan tidak dikenal\n");
    }
}

/* ------------------ Seed data ------------------ */
void seed_data_if_needed(void) {
    load_accounts("akun.txt"); load_books("buku.txt"); load_peminjaman("pinjaman.txt");
    if (jumlahAkun == 0) {
        struct Akun a; strcpy(a.username, "admin"); strcpy(a.password, "admin"); a.isAdmin = 1; daftarAkun[jumlahAkun++] = a;
        struct Akun u; strcpy(u.username, "user"); strcpy(u.password, "user"); u.isAdmin = 0; daftarAkun[jumlahAkun++] = u;
        save_accounts("akun.txt"); printf("Akun default dibuat: admin/admin (admin), user/user (user)\n");
    }
    if (jumlahBuku == 0) {
        struct Buku b1 = { nextBukuId++, "Laskar Pelangi", "Andrea Hirata", 5, 5 };
        struct Buku b2 = { nextBukuId++, "Bumi", "Tere Liye", 3, 3 };
        daftarBuku[jumlahBuku++] = b1; daftarBuku[jumlahBuku++] = b2; save_books("buku.txt");
    }
}

/* ------------------ main (entry) ------------------ */
int main(void) {
    /* init */ seed_data_if_needed();
    save_books("buku.txt"); save_accounts("akun.txt"); save_peminjaman("pinjaman.txt");
    clear_screen(); printf("====================================\n"); printf("    SISTEM PERPUSTAKAAN TERMINAL    \n"); printf("====================================\n");

    int pilihanAwal = 0;
    while (1) {
        printf("1. Register\n2. Login\n3. Keluar\nPilih: ");
        if (scanf("%d", &pilihanAwal) != 1) { printf("Input invalid\n"); break; }
        if (pilihanAwal == 1) { registerAkunUser(0); }
        else if (pilihanAwal == 2) {
            char username[32]; int isAdmin = 0;
            if (!loginUser(username, &isAdmin)) { printf("Login gagal.\n"); continue; }
            printf("Login berhasil. Hello %s\n", username);
            if (isAdmin) adminMenu(username); else userMenu(username);
        }
        else if (pilihanAwal == 3) { printf("Terima kasih. Keluar...\n"); break; }
        else printf("Pilihan tidak dikenal\n");
    }

    /* save on exit */ save_books("buku.txt"); save_accounts("akun.txt"); save_peminjaman("pinjaman.txt");
    return 0;
}
