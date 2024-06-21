/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 */
package com.mycompany.clientrmi;
import com.mycompany.projet.ICompte;

/**
 *
 * @author aelogri
 */

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.SocketException;
import java.rmi.Naming;

import java.net.DatagramSocket;

public class ClientRMI  {
    /*    public static String splitAndJoin(String input, String delimiter) {
            // Diviser la chaîne en utilisant le délimiteur
            String[] parts = input.split(delimiter);
            // Joindre les parties avec un espace
            return String.join(" ", parts);
        }*/
    public static void main(String argv[]) {
        try {
            // Vérifiez si l'adresse du serveur et le port sont passés en tant qu'arguments
            if (argv.length < 2) {
                System.out.println("Veuillez fournir l'adresse du serveur et le port en tant qu'arguments de ligne de commande.");
                return;
            }
            String serverAddress = argv[0];
            int port = Integer.parseInt(argv[1]);

            // Get a reference to the remote object named "opRect" via
            // the registry of the machine on which it is running
            ICompte compteManager = (ICompte) Naming.lookup("rmi://"+serverAddress+":1099/ServiceCompte");
             System.out.println("compiler fournir l'adresse du serveur et le port en tant qu'arguments de ligne de commande.");
            byte[] buffer = new byte[1024];
            try (DatagramSocket socket = new DatagramSocket(port)) {
                DatagramPacket packet = new DatagramPacket(buffer, buffer.length);

                while (true) {
                    socket.receive(packet);
                    String receivedData = new String(packet.getData(), 0, packet.getLength());
                    // Extraction de l'identifiant du client et du message
                    int colonIndex = receivedData.indexOf(';');
                    if (colonIndex != -1) {
                        /*String clientId = receivedData.substring(0, colonIndex);
                        String message = receivedData.substring(colonIndex + 1);

                        System.out.println("Client ID: " + clientId);
                        System.out.println("Message: " + message);*/
                        String[] parts = receivedData.split(";");
                        String clientid = parts[0].trim();
                        String action = parts[1].trim();
                        String username = parts[2].trim();
                        String password = parts[3].trim();

                        String response = processAction(compteManager, action, username, password);
                        System.out.println("Response: " + response);
                        // Concaténer l'ID du client et la réponse
                        String dataToSend = clientid + ";" + response+ ";" + username  ;
                        // Convertir la chaîne en bytes
                        byte[] dataBytes = dataToSend.getBytes();
                        // Créer un nouveau DatagramPacket
                        DatagramPacket responsePacket = new DatagramPacket(dataBytes, dataBytes.length, packet.getAddress(), packet.getPort());
                        // Envoyer le paquet
                        socket.send(responsePacket);
                    }

                    // Reset le buffer pour le prochain message
                    packet.setLength(buffer.length);
                }
            } catch (Exception e) {
                System.out.println("Error: " + e.getMessage());
                e.printStackTrace();
            }
        } catch (Exception e) {
            System.err.println(e);
        }
    }
    private static String processAction(ICompte compteManager, String action, String username, String password) throws Exception {
        switch (action) {
            case "1":
                if (compteManager.connexion(username, password)) {
                    return "user est connecte";
                } else {
                    return "erreur ";
                }
            case "4":
                if (compteManager.creerCompte(username, password)) {
                    return "Compte est bien cree";
                } else {
                    return "pseudo existe deja ";
                }
            case "5":
                if (compteManager.supprimerCompter(username, password)) {
                    return "Compte est supprimer";
                } else {
                    return "erreur de donnees";
                }
            default:
                return "Invalid action";
        }
    }
}
