#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main (int argc, char **argv) {
    char unitInput[5], unitOutput[5];
    double CPU[30];
    int size;
    double networkInput[30], networkOutput[30];
    double averageCPU = 0, averageNetworkInput = 0, averageNetworkOutput = 0;
    double varianceCPU = 0, varianceNetworkInput = 0, varianceNetworkOutput = 0 ;


    size = 0;

    while (scanf("%lf %% | %lf%s / %lf%s\n", &CPU[size], &networkInput[size], unitInput, &networkOutput[size], unitOutput) != EOF) {
      averageCPU += CPU[size];
      averageNetworkInput += networkInput[size];
      averageNetworkOutput += networkOutput[size];

      size += 1;
    }

    averageCPU /= 30;
    averageNetworkInput /= 30;
    averageNetworkOutput /= 30;

    printf("30 medições com %s clientes. 95%% de nível de confiança\n\n", argv[1]);

    printf("Quantidade média de uso de CPU foi %lf%%\n", averageCPU);
    printf("Quantidade média de entrada de bytes pela rede foi %lf%s\n", averageNetworkInput, unitInput);
    printf("Quantidade média de saída de bytes pela rede foi %lf%s\n", averageNetworkOutput, unitInput);

    for (int i = 0; i < size; i++){
      varianceCPU += (CPU[i] - averageCPU) * (CPU[i] - averageCPU);
      varianceNetworkInput += (networkInput[i] - averageNetworkInput) * (networkInput[i] - averageNetworkInput);
      varianceNetworkOutput += (networkOutput[i] - averageNetworkOutput) * (networkOutput[i] - averageNetworkOutput);
    }

    varianceCPU /= 30;
    varianceNetworkInput /= 30;
    varianceNetworkOutput /= 30;
    printf("\n\n");

    printf("Variância de uso de CPU foi %lf\n", varianceCPU);
    printf("Variância de entrada de bytes pela rede foi %lf\n", varianceNetworkInput);
    printf("Variância de saída de bytes pela rede foi %lf\n", varianceNetworkOutput);

    printf("\n\n");
    printf("Uso de CPU: Lado esquerdo: %lf lado direito: %lf\n", averageCPU - 2.04252*sqrt(varianceCPU/30), averageCPU + 2.04252*sqrt(varianceCPU/30));
    printf("Entrada de bytes pela rede : Lado esquerdo: %lf lado direito: %lf\n", averageNetworkInput - 2.04252*sqrt(varianceNetworkInput/30), averageNetworkInput + 2.04252*sqrt(varianceNetworkInput/30));
    printf("Saída de bytes pela rede: Lado esquerdo: %lf lado direito: %lf\n", averageNetworkOutput - 2.04252*sqrt(varianceNetworkOutput/30), averageNetworkOutput + 2.04252*sqrt(varianceNetworkOutput/30));
    printf("\n\n");
    return 0;
}