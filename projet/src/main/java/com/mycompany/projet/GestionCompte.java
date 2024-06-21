
package com.mycompany.projet;

import java.io.*;
import java.rmi.RemoteException;
import java.rmi.server.UnicastRemoteObject;
import java.util.Map;
import java.util.Scanner;
import java.util.TreeMap;
import java.rmi.registry.LocateRegistry;
import java.rmi.registry.Registry;

public class GestionCompte extends UnicastRemoteObject implements ICompte {

    private static final String DOSSIER_COMPTE = "liste.txt";
    private TreeMap<String, String> registreUtilisateurs;

    public GestionCompte() throws RemoteException {
        super();
        this.registreUtilisateurs = new TreeMap<>();
        lireComptes();
    }

    private void lireComptes() {
        File fichierComptes = new File(DOSSIER_COMPTE);
        try (Scanner scanner = new Scanner(fichierComptes)) {
            while (scanner.hasNextLine()) {
                String ligne = scanner.nextLine();
                if (ligne.contains("|")) {
                    String[] data = ligne.split("\\|");
                    if (data.length == 2) {
                        String pseudo = data[0].trim(); // Supprimer les espaces inutiles
                        String mdp = data[1].trim();
                        if (!pseudo.isEmpty() && !mdp.isEmpty()) {
                            registreUtilisateurs.put(pseudo, mdp);
                        } else {
                            System.err.println("Pseudo ou mot de passe vide détecté dans la ligne: " + ligne);
                        }
                    } else {
                        System.err.println("Ligne invalide, format incorrect: " + ligne);
                    }
                } else {
                    System.err.println("Délimiteur manquant dans la ligne: " + ligne);
                }
            }
        } catch (FileNotFoundException e) {
            System.err.println("Fichier non trouvé: " + e.getMessage());
        }
    }


    private void enregistrerComptes() {
        try (PrintWriter writer = new PrintWriter(new FileWriter(DOSSIER_COMPTE))) {
            for (Map.Entry<String, String> entry : registreUtilisateurs.entrySet()) {
                if (!entry.getKey().isEmpty() && !entry.getValue().isEmpty()) {
                    writer.printf("%s|%s%n", entry.getKey(), entry.getValue());
                } else {
                    System.err.println("Tentative d'enregistrer un compte avec un pseudo ou mot de passe vide.");
                }
            }
        } catch (IOException e) {
            System.err.println("Erreur lors de l'écriture dans le fichier: " + e.getMessage());
        }
    }



    @Override
    public synchronized boolean creerCompte(String pseudo, String mdp) throws RemoteException {
        if (registreUtilisateurs.containsKey(pseudo)) {
            System.err.println("Erreur: Pseudo déjà enregistré.");
            return false;
        } else {
            registreUtilisateurs.put(pseudo, mdp);
            enregistrerComptes();
            System.out.println("Nouveau compte enregistré avec succès.");
            return true;
        }
    }

    @Override
    public synchronized boolean supprimerCompter(String pseudo, String mdp) throws RemoteException {
        if (!registreUtilisateurs.containsKey(pseudo)) {
            System.err.println("Erreur: Pseudo non enregistré.");
            return false;
        } else if (!registreUtilisateurs.get(pseudo).equals(mdp)) {
            System.err.println("Erreur: Mot de passe incorrect.");
            return false;
        } else {
            registreUtilisateurs.remove(pseudo);
            enregistrerComptes();
            System.out.println("Compte supprimé avec succès.");
            return true;
        }
    }

    @Override
    public boolean connexion(String pseudo, String mdp) throws RemoteException {
        if (!registreUtilisateurs.containsKey(pseudo)) {
            System.err.println("Erreur: Pseudo non enregistré.");
            return false;
        } else if (!registreUtilisateurs.get(pseudo).equals(mdp)) {
            System.err.println("Erreur: Mot de passe incorrect.");
            return false;
        } else {
            System.out.println("Connexion réussie, Bienvenue : " + pseudo);
            return true;
        }
    }

    public static void main(String[] args) {

        try {
            GestionCompte serviceCompte = new GestionCompte();
            try {
                ICompte stub = (ICompte) UnicastRemoteObject.exportObject(serviceCompte, 0);
            } catch (java.rmi.server.ExportException ex) {
                System.err.println("Object already exported, continuing with existing object.");
            }
            LocateRegistry.createRegistry(Registry.REGISTRY_PORT);
            Registry registry = LocateRegistry.getRegistry();
            registry.rebind("ServiceCompte", serviceCompte);
            System.out.println("Service de gestion de comptes est prêt.");
        } catch (Exception e) {
            System.err.println("Erreur du serveur : " + e.toString());
            e.printStackTrace();
        }
    }
}
