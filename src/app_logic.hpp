#ifndef APP_LOGIC_HPP
#define APP_LOGIC_HPP

namespace AppLogic {
    /**
     * Contient toute la logique d'initialisation de l'application (ancien setup()).
     */
    void initialize();

    /**
     * Contient toute la logique d'exécution cyclique (ancien loop()).
     */
    void execute();
}

#endif