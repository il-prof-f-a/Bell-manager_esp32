**Documentazione e esempio Json**  
 **Bell-Manager**

Questo Json  rappresenta il Database completo dell’applicazione e permette di gestire: campanelle programmate, impostazioni globali e log delle campanelle già suonate. Si può così Salvare,Eliminare,Leggere,Modificare i dati.

Il Json è diviso in 3 sezioni: 

* campanelle  
* impostazioni  
* logCampanelle

* campanelle:  
  * Array che mi permette di gestire tutte le campanelle che l’utente crea, ogni campanella è un oggetto indipendente:

 campanelle:   
\[  
    {  
      id: “1”,  
      ora: "08:00",  
      durata: ”22”,  
      tipo: "inizio lezione",  
      giorni: {  
        lun: “true”,  
        mar: “true”,  
        mer: “true”,  
        gio: “true”,  
        ven: “true”,  
        sab: “false”,  
        dom: “false”  
      },  
      stato: “true”  
    },

| Campo | Descrizione |
| :---: | :---: |
| Id | Id univoco della campanella |
| Ora | Orario impostato |
| Durata | Durata della campanella in secondi |
| Tipo  | Tipo della campanella |
| Giorni | Giorni della settimana in cui è attiva la campanella, ogni giorno è un boolean in modo da caricarlo facilmente nello switch |
| Stato | Stato della campanella ( true: attiva,false: spenta) |

* impostazioni:  
  * Oggetto unico che contiene le impostazioni globali dell’app.

 impostazioni:   
{  
    nomeIstituzione: "Istituto Tecnico Franchetti Salviani",  
    disattivatutteCampanelle: “false”

 },

| Campo  | Descrizione |
| :---: | ----- |
|  nomeIstituzione | Nome dell’istituto String, mostrato nell’interfaccia. |
| disattivatutteCampanelle | Switch globale booleano che disattiva/attiva tutte le campanelle:  Se è true: tutte le campanelle vengono considerate OFF (anche se singolarmente attive). Se false: vale lo stato di ogni campanella. |

* logcampanelle: Array che contiene lo storico delle campanelle che hanno suonato, limitato agli ultimi 20 eventi.


  logCampanelle:  
 \[  
    {  
      idCampanella: “1”,  
      tipo: "inizio lezione",  
      ora: "08:00",  
      giorno: "lun",  
      timestamp: "05-02-2026"  
    },

| Campo  | Descrizione |
| :---: | ----- |
| idCampanella | ID della campanella che ha suonato |
| tipo | Tipo della campanella |
| ora | Ora in cui è suonata la campanella |
| giorno | Giorno della settimana |
| timestamp | Data o ora  |

ESEMPIO DI JSON:

**{**  
    
  campanelle:   
\[  
    {  
      id: “1”,  
      ora: "08:00",  
      durata: ”22”,  
      tipo: "inizio lezione",  
      giorni: {  
        lun: “true”,  
        mar: “true”,  
        mer: “true”,  
        gio: “true”,  
        ven: “true”,  
        sab: “false”,  
        dom: “false”  
      },  
      stato: “true”  
    },  
    {  
      id:” 2”,  
      ora: "10:55",  
      durata: “15”,  
      tipo: "intervallo",  
      giorni:  
       {  
        lun: “true”,  
        mar: “true”,  
        mer: “true”,  
        gio: “true”,  
        ven: “true”,  
        sab: “false”,  
        dom: “false”  
      },  
      stato:” false”  
    }  
  \],

 impostazioni:   
{  
    nomeIstituzione: "Istituto Tecnico Franchetti Salviani",  
    disattivatutteCampanelle: “false”

 },

  logCampanelle:  
 \[  
    {  
      idCampanella: “1”,  
      tipo: "inizio lezione",  
      ora: "08:00",  
      giorno: "lun",  
      timestamp: "05-02-2026"  
    },  
    {  
      idCampanella:” 2”,  
      tipo: "intervallo",  
      ora: "10:55",  
      giorno: “lun",  
      timestamp: "05-02-2026"  
    }  
  \]  
}