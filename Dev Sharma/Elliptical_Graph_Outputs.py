import numpy as np
import matplotlib.pyplot as plt
from mpmath import ellipk, ellippi
import mpmath
from concurrent.futures import ProcessPoolExecutor, as_completed
from tqdm import tqdm
import pandas as pd
from tabulate import tabulate

mpmath.mp.dps = 15  # Decimal precision


# Constants
a_max_cm = 4.0
t_w_cm = 0.064
col_l_cm = 8.0
t_p_cm = 1.0
mu0 = 4 * np.pi * 1e-7
B_r = 1.2
density = 7400
g = 9.8

current_increment = 0.2
# max_curr removed to allow unlimited increment until reaching F_MAX
r_samplesN = 30
z_samplesN = 10


def elliptic_vals(k2, h, cache):
    key = (float(k2), float(h))
    if key in cache:
        return cache[key]
    K_val = float(ellipk(k2))
    Pi_val = float(ellippi(h, k2))
    cache[key] = (K_val, Pi_val)
    return K_val, Pi_val


def x_elliptic(temp, r, a, cache):
    k = np.sqrt(4 * a * r / ((r + a) ** 2 + temp ** 2))
    h = 4 * a * r / (r + a) ** 2
    K, Pi = elliptic_vals(k ** 2, h, cache)
    numerator = temp / np.sqrt((r + a) ** 2 + temp ** 2)
    bracket = ((r - a) / (r + a)) * Pi - K
    return numerator * bracket


def Bz_function(r_arr, z, a, N, I, coil_len, cache):
    z1 = z + coil_len / 2
    z2 = z - coil_len / 2
    prefactor = mu0 * N * I / (2 * np.pi * coil_len)
    return np.array([prefactor * (x_elliptic(z2, r, a, cache) - x_elliptic(z1, r, a, cache)) for r in r_arr])


def compute_for_length_a_core(args):
    a_core_cm, l_cm, r_id_cm, r_od_cm = args

    cache = {}
    a_core = a_core_cm / 100.0
    a_max = a_max_cm / 100.0
    t_w = t_w_cm / 100.0
    l = l_cm / 100.0
    col_l = col_l_cm / 100.0
    r_id = r_id_cm / 100.0
    r_od = r_od_cm / 100.0
    t_p = t_p_cm / 100.0
    z_actual = (l / 2) + 0.005 + (col_l / 2)
    N_o = int(l / t_w)
    N_l = int((a_max - a_core) / t_w)

    V = np.pi * t_p * (r_od ** 2 - r_id ** 2)
    m = density * V
    F_MAX = (m * (20 * g + g)) / 2

    r_samples = np.linspace(r_id, r_od, r_samplesN)
    Nz = z_samplesN
    z_samples = np.linspace(z_actual - t_p / 2, z_actual + t_p / 2, Nz)
    M_r = B_r / mu0

    current = current_increment
    F_at_I = None
    while True:  # No max current cap, run until reaching F_MAX
        F_total = 0.0
        for layer in range(N_l):
            a_layer = a_core + t_w * layer + t_w / 2
            integrand_matrix = np.zeros((Nz, len(r_samples)))
            for i, z_val in enumerate(z_samples):
                dBz_dz_vals = (Bz_function(r_samples, z_val + 1e-7, a_layer, N_o, current, l, cache) -
                               Bz_function(r_samples, z_val - 1e-7, a_layer, N_o, current, l, cache)) / (2 * 1e-7)
                integrand_matrix[i, :] = dBz_dz_vals * r_samples
            radial_integration = np.trapz(integrand_matrix, r_samples, axis=1)
            double_integral = np.trapz(radial_integration, z_samples)
            F_layer = M_r * 2 * np.pi * double_integral
            F_total += F_layer
        if abs(F_total) >= F_MAX:
            F_at_I = F_total
            break
        current += current_increment

    return (a_core_cm, l_cm, N_o, N_l, current, F_MAX)


if __name__ == "__main__":
    # User input for r_id and r_od
    r_id_cm = float(input("Enter inner radius r_id (cm): "))
    r_od_cm = float(input("Enter outer radius r_od (cm): "))

    a_core_values = np.arange(0.5, 2.6, 0.5)
    l_cm_values = range(4, 11)
    inputs = [(a, l, r_id_cm, r_od_cm) for a in a_core_values for l in l_cm_values]

    results = []
    with ProcessPoolExecutor() as executor:
        futures = [executor.submit(compute_for_length_a_core, args) for args in inputs]
        for future in tqdm(as_completed(futures), total=len(futures), desc="Computing"):
            results.append(future.result())

    # Use F_MAX from the first result (all are same for given inputs)
    _, _, _, _, _, F_MAX = results[0]

    # Organize results to plot
    import collections
    results_by_core = collections.defaultdict(list)
    for a_core_cm, l_cm, N_o, N_l, I_max, F_val in results:
        results_by_core[a_core_cm].append((N_o, I_max, l_cm, N_l))

    plt.figure(figsize=(12, 8))

    rows = []
    for a_core_cm, data_list in results_by_core.items():
        data_list.sort(key=lambda x: x[0])  # sort by N_0
        for (N0, I, l, Nl) in data_list:
            rows.append([a_core_cm, l, N0, Nl, I])
        N0_vals = [d[0] for d in data_list]
        I_vals = [d[1] if d[1] is not None else 0 for d in data_list]
        Nl_vals = [d[3] for d in data_list]

        label = f'$a_{{core}}={a_core_cm}cm, N_l={Nl_vals[0]}$'
        plt.plot(N0_vals, I_vals, marker='o', label=label)

        # Annotate max current
        for (N0, I, l, Nl) in data_list:
            if I is not None:
                plt.annotate(f"{I:.2f}", (N0, I),
                             textcoords="offset points", xytext=(0, 8), ha='center', fontsize=7)

    plt.xlabel('Number of Turns per Layer $N_0$')
    plt.ylabel('Minimum Current $I_{max}$ (A) to Reach $F_{MAX}$')
    plt.title(f'$N_0$ vs $I_{{max}}$ for $r_{{id}}={r_id_cm}$cm, $r_{{od}}={r_od_cm}$cm, $F_{{MAX}}={F_MAX:.2f}$N')
    plt.legend(title='Core Radius and Layers')
    plt.grid(True)
    plt.tight_layout()
    plt.show()

    # Print data table immediately after plotting
    df = pd.DataFrame(rows, columns=['a_core_cm', 'l_cm', 'N_o', 'N_l', 'I_max'])

    # Print table using tabulate for fast clear output
    print(tabulate(df, headers='keys', tablefmt='psql', showindex=False))
