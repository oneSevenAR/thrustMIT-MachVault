import numpy as np
import matplotlib.pyplot as plt
from mpmath import ellipk, ellippi
import mpmath
import time
from concurrent.futures import ProcessPoolExecutor, as_completed
from tqdm import tqdm

mpmath.mp.dps = 15  # Decimal precision

# Constants
a_max_cm = 4.0
t_w_cm = 0.064
col_l_cm = 8.0
r_id_cm = 1.5
r_od_cm = 3.0
t_p_cm = 1.0
mu0 = 4 * np.pi * 1e-7
B_r = 1.2
density = 7400
g = 9.8

current_increment = 0.2
max_curr = 10.0
r_samplesN = 30
z_samplesN = 10

elliptic_cache = {}

def elliptic_vals(k2, h):
    key = (float(k2), float(h))
    if key in elliptic_cache:
        return elliptic_cache[key]
    K_val = float(ellipk(k2))
    Pi_val = float(ellippi(h, k2))
    elliptic_cache[key] = (K_val, Pi_val)
    return K_val, Pi_val

def x_elliptic(temp, r, a):
    k = np.sqrt(4*a*r / ((r+a)**2 + temp**2))
    h = 4*a*r / (r+a)**2
    K, Pi = elliptic_vals(k**2, h)
    numerator = temp / np.sqrt((r + a)**2 + temp**2)
    bracket = ((r - a) / (r + a)) * Pi - K
    return numerator * bracket

def Bz_function(r_arr, z, a, N, I, coil_len):
    z1 = z + coil_len / 2
    z2 = z - coil_len / 2
    prefactor = mu0 * N * I / (2 * np.pi * coil_len)
    return np.array([prefactor * (x_elliptic(z2, r, a) - x_elliptic(z1, r, a)) for r in r_arr])

def compute_for_length_a_core(args):
    a_core_cm, l_cm = args

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

    V = np.pi * t_p * (r_od**2 - r_id**2)
    m = density * V
    F_MAX = (m * (20 * g + g)) / 2

    r_samples = np.linspace(r_id, r_od, r_samplesN)
    Nz = z_samplesN
    z_samples = np.linspace(z_actual - t_p / 2, z_actual + t_p / 2, Nz)
    M_r = B_r / mu0

    found = False
    current = current_increment
    F_at_I = None
    while current <= max_curr:
        F_total = 0.0
        for layer in range(N_l):
            a_layer = a_core + t_w * layer + t_w / 2
            integrand_matrix = np.zeros((Nz, len(r_samples)))
            for i, z_val in enumerate(z_samples):
                dBz_dz_vals = (Bz_function(r_samples, z_val + 1e-7, a_layer, N_o, current, l) -
                               Bz_function(r_samples, z_val - 1e-7, a_layer, N_o, current, l)) / (2 * 1e-7)
                integrand_matrix[i, :] = dBz_dz_vals * r_samples
            radial_integration = np.trapz(integrand_matrix, r_samples, axis=1)
            double_integral = np.trapz(radial_integration, z_samples)
            F_layer = M_r * 2 * np.pi * double_integral
            F_total += F_layer
        if abs(F_total) >= F_MAX:
            found = True
            F_at_I = F_total
            break
        current += current_increment

    if not found:
        current = None
        F_at_I = None

    return (a_core_cm, l_cm, N_o, N_l, current, F_at_I)


if __name__ == "__main__":
    a_core_values = np.arange(0.5, 2.6, 0.5)
    l_cm_values = range(4, 11)
    inputs = [(a, l) for a in a_core_values for l in l_cm_values]

    start_time = time.time()

    results = []
    with ProcessPoolExecutor() as executor:
        futures = [executor.submit(compute_for_length_a_core, args) for args in inputs]
        for future in tqdm(as_completed(futures), total=len(futures), desc="Computing"):
            results.append(future.result())

    end_time = time.time()
    print(f"Parallel computation completed in {end_time - start_time:.2f} seconds")

    plt.figure(figsize=(12, 8))

    import collections
    results_by_core = collections.defaultdict(list)
    for a_core_cm, l_cm, N_o, N_l, I_max, F_val in results:
        results_by_core[a_core_cm].append((N_o, I_max, l_cm, N_l, F_val))

    for a_core_cm, data_list in results_by_core.items():
        data_list.sort(key=lambda x: x[0])  # sort by N_o (N_0)
        N0_vals = [d[0] for d in data_list]
        I_vals = [d[1] if d[1] is not None else max_curr for d in data_list]
        Nl_vals = [d[3] for d in data_list]
        F_vals = [d[4] for d in data_list]

        label = f'$a_{{core}}={a_core_cm}cm, N_l={Nl_vals[0]}$'
        plt.plot(N0_vals, I_vals, marker='o', label=label)

        for (N0, I, l, Nl, F) in data_list:
            if I is not None and F is not None:
                # Annotate with current and force values
                plt.annotate(f"I:{I:.2f}\nF:{F:.2f}N", (N0, I),
                             textcoords="offset points", xytext=(0, 8), ha='center', fontsize=7)

    plt.xlabel('Number of Turns per Layer $N_0$')
    plt.ylabel('Minimum Current $I_{max}$ (A) to Reach $F_{MAX}$')
    plt.title('$N_0$ vs $I_{max}$ with Force and Layer Count')
    plt.legend(title='Core Radius and Layers')
    plt.grid(True)
    plt.tight_layout()
    plt.show()
    