import os
import subprocess
import tkinter as tk
from tkinter import filedialog, messagebox

# Dossier contenant ce script Python et l'émulateur
DOSSIER_SCRIPT = os.path.dirname(os.path.abspath(__file__))
EMULATEUR = os.path.join(DOSSIER_SCRIPT, "tinyjoypad_emu")

# Dossier initial du sélecteur de fichiers
dernier_dossier_hex = DOSSIER_SCRIPT


def choisir_hex():
    global dernier_dossier_hex

    chemin = filedialog.askopenfilename(
        title="Sélectionner un fichier HEX",
        initialdir=dernier_dossier_hex,
        filetypes=[
            ("Fichiers HEX", "*.hex"),
            ("Tous les fichiers", "*.*")
        ]
    )

    if chemin:
        chemin = os.path.abspath(chemin)
        fichier_hex_var.set(chemin)
        dernier_dossier_hex = os.path.dirname(chemin)


def lancer():
    fichier_hex = fichier_hex_var.get().strip()

    if not fichier_hex or not os.path.isfile(fichier_hex):
        messagebox.showerror(
            "Fichier HEX manquant",
            "Sélectionne un fichier .hex valide."
        )
        return

    if not os.path.isfile(EMULATEUR):
        messagebox.showerror(
            "Émulateur introuvable",
            f"Emplacement vérifié :\n{EMULATEUR}"
        )
        return

    try:
        subprocess.Popen(
            [EMULATEUR, fichier_hex],
            cwd=DOSSIER_SCRIPT
        )
    except OSError as erreur:
        messagebox.showerror("Erreur au lancement", str(erreur))


fenetre = tk.Tk()
fenetre.title("TinyJoypad — lanceur HEX")
fenetre.resizable(False, False)

fichier_hex_var = tk.StringVar()

tk.Label(fenetre, text="Fichier .hex :").grid(
    row=0, column=0, padx=10, pady=12, sticky="w"
)

tk.Entry(fenetre, textvariable=fichier_hex_var, width=50).grid(
    row=0, column=1, padx=5, pady=12
)

tk.Button(fenetre, text="Parcourir…", command=choisir_hex).grid(
    row=0, column=2, padx=10, pady=12
)

tk.Button(fenetre, text="Exécuter", command=lancer).grid(
    row=1, column=1, pady=(0, 12)
)

fenetre.mainloop()
