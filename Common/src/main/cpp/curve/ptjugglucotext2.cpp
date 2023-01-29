/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2 and 3 sensors.                         */
/*                                                                                   */
/*      Copyright (C) 2021 Jaap Korthals Altes <jaapkorthalsaltes@gmail.com>         */
/*                                                                                   */
/*      Juggluco is free software: you can redistribute it and/or modify             */
/*      it under the terms of the GNU General Public License as published            */
/*      by the Free Software Foundation, either version 3 of the License, or         */
/*      (at your option) any later version.                                          */
/*                                                                                   */
/*      Juggluco is distributed in the hope that it will be useful, but              */
/*      WITHOUT ANY WARRANTY; without even the implied warranty of                   */
/*      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                         */
/*      See the GNU General Public License for more details.                         */
/*                                                                                   */
/*      You should have received a copy of the GNU General Public License            */
/*      along with Juggluco. If not, see <https://www.gnu.org/licenses/>.            */
/*                                                                                   */
/*      Fri Jan 27 15:20:04 CET 2023                                                 */


include "jugglucotext.h"
#ifndef WEAROS
constexpr static std::string_view labels[]={"Insuli Rápida","Carbohidrato","Glicemia","Insuli Lenta","Bicicleta","Caminhada","Sangue"};
constexpr static Shortcut_t  shortinit[]= { {"Pão",
        .48},
        {"Currantbun1",
        .56f},
        {"Currantbun2",
        .595f},
        {"Uvas",
        .165f},
        {"IogPedaços",
        .058f},
        {"Arroz",
        .75f},
        {"Macarrão",
        .65f},
        {"Tomate",
        .03f},
        {"Mexicano",
        .078f},
        {"SumoLaranja",
        .109f},
        {"PóDesportivo",
        .873f},
        {"Mix(Cenoura)",
        .07f},
        {"Cogumelo",
        .07300000f}};
#endif

extern jugglucotext engtext;
jugglucotext engtext {
        .daylabel={"Dom","Seg","Ter","Qua","Qui","Sex","Sáb"},
        .monthlabel={
      "Jan","Feb","Mar","Abr","Mai"      ,             "Jun",
       "Jul","Ago","Set",
      "Out","Nov","Dez"},

        .scanned="Digitalizado",
#ifndef WEAROS
        .median="Mediana",
        .middle="Média",
#endif
        .history="Histórico",
        .historyinfo="Uma vez a cada 15 minutos.\nRelembrar por 8 hours.\nA digitalização transfere-os para este programa.\nSensor: ",
        .history3info=" Uma vez a cada 5 minutes.\nRRelembrar por 14 dias.\nTransferido por Bluetooth para este programa.\nSensor: ",
        .sensorstarted= "Sensor iniciou:",
        .lastscanned="Último scanned:",
        .laststream="Último stream:",
        .sensorends="Sensor termina: ",
#ifndef WEAROS
        .newamount="Novo valor",
        .averageglucose="Glicose Média: ",
        .duration="Duração: %.1f dias",
        .timeactive="%.1f%% de tempo ativo",
        .nrmeasurement="Número de medições: %d",
        .EstimatedA1C="A1C estimada: %.1f%% (%d mmol/mol)",
        .GMI="Indicador de Gerenciamento de Glicose: %.1f%% (%d mmol/mol)",
        .SD="SD: %.2f",
        .glucose_variability="Variabilidade da glicose: %.1f%%",
     .menustr0={
                "UI do Sistema",
                "Relógio",
                "Sensor",
                "Definições",
                "Sobre",
                "Fechar",
                "Parar Alarme"
                },
        .menustr1={
                "Notificar",
                "Exportar",
                "Espelhar",
                engtext.newamount,
                "Listagem", 
                "Estatísticas"
                },
        .menustr2= {"Último Scan","Scans","Stream","Histórico","Valores","Refeições","Modo escuro        "},
        .menustr3= {hourminstr,"Procurar","Data","Dia anterior","Dia seguinte","Semana anterior","Semana seguinte"},
#else
 .amount="Valor",
 .menustr0= {
        "Espelhar",
        "Sensor",
        "Modo escuro     ",
        "Definições",
        "Parar Alarme" },
.menustr2= {"Data",hourminstr,"Dia anterior",engtext.amount},
#endif

        .scanerrors={
                {"Scan Erro (%d)","Tentar de novo"},
                {"Erro de instalação","?"},
                {"Erro no processamento de dados","Tentar de novo"},
                {"Ativar Sensor",""},
                {"Sensor terminou definitivamente",""},
                {"Sensor pronto em","%d minutos"},
                {"373: Erro do Sensor","Não contacte imediatamente o serviço de apoio ao cliente Abbott; possivelmente as leituras de glicose estarão disponíveis em 10 minutos."},
                {"Novo Sensor iniciado","Scan novamente para o utilizar"},
                {"","Bloquear toques durante o scan"},
                {"",""},
                {"Erro de iniciação da biblioteca","Forçar reinstalação removendo a biblioteca"},
                {"Erro de inicialização de classificação","Fazer algo"},
                {"O procedimento demora demasiado tempo","Fechar programa"},
                {"365: Substituir Sensor","O seu Sensor não funciona. Por favor remova o seu Sensor e inicie um novo."},
                {"368: Substituir Sensor","O seu Sensor não está a funcionar. Por favor remova o seu Sensor e inicie um novo."},
                {"",""},
                {"Scan Erro","Tentar novamente"}},

#ifndef WEAROS
        .advancedstart= R"(<h1>Dispositivo modificado</h1>
<p>Uma das bibliotecas utilizadas por esta aplicação tem um BUG que faz
que feche se detectar certos ficheiros. O seu dispositivo contém alguns dos seguintes ficheiros. Este programa contém um hack para contornar este BUG, mas
é provavelmente melhor tornar estes ficheiros indetetáveis de algum modo. Magisk, por exemplo, tem a opção de esconder o root para certas
aplicações (Magiskhide ou Denylist) e mudar o seu próprio nome, ambos 
são necessárias. No seu caso, tem problemas com o seguinte ficheiro)",
        .add_s=true,
.shortinit=shortinit,
.labels=labels
#endif
                }


                ;

jugglucotext *usedtext= &engtext;

void setuseeng() {
 usedtext= &engtext;
 }

