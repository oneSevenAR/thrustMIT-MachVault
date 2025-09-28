import numpy as np
import matplotlib.pyplot as plt
from mpmath import ellipk, ellippi
import mpmath
from scipy.integrate import quad, dblquad
from concurrent.futures import ProcessPoolExecutor, as_completed
from tqdm import tqdm

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

def Bz_function_single(r, z, a, N, I, coil_len):
    z1 = z + coil_len / 2
    z2 = z - coil_len / 2
    prefactor = mu0 * N * I / (2 * np.pi * coil_len)
    return prefactor * (x_elliptic(z2, r, a) - x_elliptic(z1, r, a))

def dBz_dz_single(r, z, a, N, I, coil_len, dz=1e-7):
    return (Bz_function_single(r, z + dz, a, N, I, coil_len) - 
            Bz_function_single(r, z - dz, a, N, I, coil_len)) / (2 * dz)

def compute_for_length_a_core(args):
    a_core_cm, l_cm, r_id_cm, r_od_cm = args

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
    F_MAX = (m * (20*g + g))/2

    den_int = 0.5*(r_od**2 - r_id**2)
    z_min = z_actual - t_p/2
    z_max = z_actual + t_p/2
    M_r = B_r / mu0

    current = current_increment
    while True:
        F_total = 0.0
        B_total = 0.0

        for layer in range(N_l):
            a_layer = a_core + t_w*layer + t_w/2

            # full integral for weighted B
            def B_integrand(r):
                return Bz_function_single(r, z_actual, a_layer, N_o, current, l) * r
            B_integral, _ = quad(B_integrand, r_id, r_od, epsabs=1e-10, epsrel=1e-10)
            B_avg = B_integral / den_int

            # full double integral for force
            def force_integrand(z, r):
                return dBz_dz_single(r, z, a_layer, N_o, current, l) * r
            double_integral, _ = dblquad(
                force_integrand,
                r_id, r_od,
                lambda r: z_min, lambda r: z_max,
                epsabs=1e-10, epsrel=1e-10
            )
            F_layer = M_r * 2*np.pi * double_integral

            B_total += B_avg
            F_total += F_layer

        if abs(F_total) >= F_MAX:
            break
        current += current_increment

    return (a_core_cm, l_cm, N_o, N_l, current)

if __name__ == "__main__":
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

    # Organize results for plotting
    import collections
    results_by_core = collections.defaultdict(list)
    for a_core_cm, l_cm, N_o, N_l, I_max in results:
        results_by_core[a_core_cm].append((N_o, I_max, l_cm, N_l))

    plt.figure(figsize=(12,8))
    for a_core_cm, data_list in results_by_core.items():
        data_list.sort(key=lambda x: x[0])
        N0_vals = [d[0] for d in data_list]
        I_vals = [d[1] for d in data_list]
        label = f'$a_{{core}}={a_core_cm}cm, N_l={data_list[0][3]}$'
        plt.plot(N0_vals, I_vals, marker='o', label=label)
        # annotate each point with its current
        for N0, I, l, Nl in data_list:
            plt.annotate(f"{I:.2f}", (N0, I), textcoords="offset points", xytext=(0,8), ha='center', fontsize=7)

    plt.xlabel('Number of Turns per Layer $N_0$')
    plt.ylabel('Minimum Current $I_{max}$ (A) to Reach $F_{MAX}$')
    plt.title(f'$N_0$ vs $I_{{max}}$ for $r_{{id}}={r_id_cm}$cm, $r_{{od}}={r_od_cm}$cm')
    plt.legend(title='Core Radius and Layers')
    plt.grid(True)
    plt.tight_layout()
    plt.show()
