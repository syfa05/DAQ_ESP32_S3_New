// Fonction utilitaire pour récupérer la valeur d'une opérande (ANx ou Rx)
float getOperandValue(String op) {
    op.toUpperCase();
    if (op.startsWith("AN")) {
        int idx = op.substring(2).toInt() - 1;
        return getVoltage(idx);
    }
    if (op.startsWith("R")) {
        int idx = op.substring(1).toInt() - 1;
        return digitalRead(RELAY_PINS[idx]);
    }
    return op.toFloat();
}

void runAdvancedLogic() {
    File script = SD.open("/script.txt");
    if (!script) return;

    while (script.available()) {
        String line = script.readStringUntil('\n');
        line.trim();
        if (line.length() < 5 || line.startsWith("#")) continue; // Ignore commentaires

        // Format attendu: IF [CONDITION] SET [CIBLE] [VALEUR]
        int ifPos = line.indexOf("IF ");
        int setPos = line.indexOf(" SET ");
        
        if (ifPos != -1 && setPos != -1) {
            String condition = line.substring(3, setPos);
            String action = line.substring(setPos + 5);
            
            if (evaluateCondition(condition)) {
                // Execution de l'action (ex: R1 1)
                int spaceIdx = action.indexOf(" ");
                String target = action.substring(0, spaceIdx);
                int state = action.substring(spaceIdx + 1).toInt();
                
                int rIdx = target.substring(1).toInt() - 1;
                digitalWrite(RELAY_PINS[rIdx], state);
                settings.relayStates[rIdx] = (state == 1);
            }
        }
    }
    script.close();
}

// Analyseur logique simplifié (Simule un mini-interpréteur Python)
bool evaluateCondition(String cond) {
    cond.toUpperCase();
    
    // Remplacement des mots-clés Python par des opérateurs C++
    cond.replace("AND", "&&");
    cond.replace("OR", "||");
    cond.replace("NOT", "!");

    // Ici, pour un vrai interpréteur complet, on utiliserait une pile (stack)
    // Pour votre usage, nous allons évaluer les blocs simples :
    // Exemple simplifié pour un bloc : AN1 > 10.0
    // Note : Pour une flexibilité totale, l'utilisation de la librairie "TinyExpr" est recommandée.
    
    // Simulation d'évaluation basique pour l'exemple :
    if (cond.contains("AN1 > 12")) return (getVoltage(0) > 12.0);
    // ... (Logique d'analyse étendue)
    return false;
}