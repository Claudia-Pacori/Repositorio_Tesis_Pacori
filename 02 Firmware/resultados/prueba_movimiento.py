import pandas as pd
import matplotlib.pyplot as plt

def process_data(file_path):
    data = []
    with open(file_path, "r") as file:
        for line in file:
            if line.strip() and not(line.startswith("#")):  # Ignorar comentarios
                params, metrics = line.split("|")
                params = params.strip()
                metrics = metrics.strip()
                
                # Extraer parametros
                tp = int(params.split(",")[0].split(":")[1].strip())
                sf = int(params.split(",")[1].split(":")[1].strip())
                cr = int(params.split(",")[2].split(":")[1].strip())
                
                # Extraer indicadores
                rssi = float(metrics.split(",")[0].split(":")[1].strip().replace("dBm", ""))
                snr = float(metrics.split(",")[1].split(":")[1].strip().replace("dB", ""))
                pdr = float(metrics.split(",")[2].split(":")[1].strip().replace("%", ""))
                current = float(metrics.split(",")[3].split(":")[1].strip().replace("mA", ""))
                
                data.append([tp, sf, cr, rssi, snr, pdr, current])
    
    columns = ["TP", "SF", "CR", "RSSI (dBm)", "SNR (dB)", "PDR (%)", "Corriente (mA)"]
    df = pd.DataFrame(data, columns=columns)
    df["ID"] = range(1, len(df) + 1)
    return df

df1 = process_data("resultados/1hour.txt")

def plot_show_all(indicator_metrics, param_metrics, dfs, labels, colors):
    # Crear una figura grande con subgráficas
    fig1, axs = plt.subplots(4, 1, figsize=(18, 16))
    
    for i, (metric, ylabel) in enumerate(indicator_metrics):
        ax = axs[i]
        for df, label, color in zip(dfs, labels, colors):
            ax.plot(df["ID"], df[metric], marker="o", label=label, color=color)
        ax.set_title(metric)
        ax.set_xlabel("ID")
        ax.set_ylabel(ylabel)
        ax.grid(True)
        ax.legend()
    
    fig1.suptitle("Indicadores de Calidad")
    fig1.tight_layout()
    fig1.subplots_adjust(top=0.9)

    # Crear una figura separada para los parámetros
    fig2, axs = plt.subplots(3, 1, figsize=(18, 10))

    for i, (metric, ylabel) in enumerate(param_metrics):
        ax = axs[i]
        for df, label, color in zip(dfs, labels, colors):
            ax.plot(df["ID"], df[metric], marker="o", label=label, color=color)
        ax.set_title(metric)
        ax.set_xlabel("ID")
        ax.set_ylabel(ylabel)
        ax.grid(True)
        ax.legend()
    
    fig2.suptitle("Parámetros de Configuración")
    fig2.tight_layout()
    fig2.subplots_adjust(top=0.9)
    plt.show()

dfs = [df1]
labels = ["Punto1"]
colors = ["#1b9aaa"]

# Listas de métricas
indicator_metrics = [
    ("RSSI (dBm)", "dBm"),
    ("SNR (dB)", "dB"),
    ("PDR (%)", "%"),
    ("Corriente (mA)", "mA")
]

param_metrics = [
    ("TP", "dBm"),
    ("SF", ""),
    ("CR", "")
]

# Mostrar todos los gráficos
plot_show_all(indicator_metrics, param_metrics, dfs, labels, colors)