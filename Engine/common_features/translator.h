/*
 * Platformer Game Engine by Wohlstand, a free platform for game making
 * Copyright (c) 2017 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PGE_TRANSLATOR_H
#define PGE_TRANSLATOR_H

#include "QTranslatorX/qm_translator.h"

class PGE_Translator
{
public:
    PGE_Translator();
    void init();
    void toggleLanguage(std::string lang);
private:
    friend std::string qsTrId(const char* string);

    bool            m_isInit;
    //QTranslator     m_translator;   /**< contains the translations for this application */
    //QTranslator     m_translatorQt; /**< contains the translations for qt */
    QmTranslatorX   m_translator;   /**< contains the translations for this application */
    std::string     m_currLang;     /**< contains the currently loaded language */
    std::string     m_langPath;     /**< Path of language files. This is always fixed to /languages. */
};

#endif // PGE_TRANSLATOR_H
