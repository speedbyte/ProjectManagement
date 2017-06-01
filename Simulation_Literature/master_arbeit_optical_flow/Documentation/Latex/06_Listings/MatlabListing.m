Zeitvektor = Daten_1konstant_2Sinus.time;
Signal_01  = Daten_1konstant_2Sinus.signals(1).values;
Signal_02  = Daten_1konstant_2Sinus.signals(2).values;

clf %Clear current figure

figure
title('Signal 1','FontSize',12);

subplot(2,2,[1 2]);
plot(Zeitvektor,Signal_01,'linewidth',2)
grid on
axis([0, 3, 0, 2])
title('Signal 1','FontSize',12);
xlabel('Zeit in s','FontSize',12)
ylabel('Position in m','FontSize',12)

subplot(2,2,[3 4]);
plot(Zeitvektor,Signal_02,'linewidth',2)
grid on
axis([0, 3, 0, 2])
title('Signal 2','FontSize',12);
xlabel('Zeit in s','FontSize',12)
ylabel('Position in m','FontSize',12)

%----------------Diagramm als png-Bild speichern----------
cd C:\ %Arbeitsverzeichnis festlegen
print -dpng -r300 Dateiname %png Bild mit 300dpi Auflösung

%absolutdimesnionen efstlegen
%gesamttitel
%Foirmatierungen (schriftgröße etc.)
