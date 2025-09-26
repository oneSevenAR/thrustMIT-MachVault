import numpy as np
import pandas as pd
from mpmath import ellipk, ellippi
import mpmath
import time

mpmath.mp.dps = 15  # Decimal precision

# ========== INPUTS IN CENTIMETERS ==========
l_cm = 6.0         # Coil length in cm
a_core_cm = 1.0    # Core radius in cm
a_max_cm = 4.0     # Max winding radius in cm
t_w_cm = 0.064     # Wire thickness in cm
col_l_cm = 8.0     # Fixed column length in cm

r_id_cm = 1.5      # PM inner radius in cm
r_od_cm = 3.0      # PM outer radius in cm
t_p_cm = 1.0       # PM thickness in cm

# ========== CONSTANTS AND CONVERSIONS TO METERS ==========
mu0 = 4 * np.pi * 1e-7

l = l_cm / 100.0
a_core = a_core_cm / 100.0
a_max = a_max_cm / 100.0
t_w = t_w_cm / 100.0
col_l = col_l_cm / 100.0

r_id = r_id_cm / 100.0
r_od = r_od_cm / 100.0
t_p = t_p_cm / 100.0

z_actual = (l / 2) + 0.005 + (col_l / 2)

B_r = 1.2
density = 7400
g = 9.8  # m/s^2

current_increment = 0.2

M_r = B_r / mu0
V = np.pi * t_p * (r_od**2 - r_id**2)
m = density * V

# Compute F_MAX
F_MAX = (m * (20 * g + g)) / 2
print(f"F_MAX = {F_MAX:.4f} N")

N_o = int(l / t_w)
N_l = int((a_max - a_core) / t_w)

r_samples = np.linspace(r_id, r_od, 30)
den_int = 0.5 * (r_od**2 - r_id**2)

Nz = 20
z_samples = np.linspace(z_actual - t_p / 2, z_actual + t_p / 2, Nz)

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
    Bz_vals = np.array([prefactor * (x_elliptic(z2, r, a) - x_elliptic(z1, r, a)) for r in r_arr])
    return Bz_vals

start_time = time.time()

results = []
current = current_increment
stop_current = None

while True:
    B_total = 0.0
    F_total = 0.0

    for layer in range(N_l):
        a_layer = a_core + t_w * layer + t_w / 2

        Bz_vals = Bz_function(r_samples, z_actual, a_layer, N_o, current, l)
        B_avg = np.trapz(Bz_vals * r_samples, r_samples) / den_int

        integrand_matrix = np.zeros((Nz, len(r_samples)))
        for i, z_val in enumerate(z_samples):
            dBz_dz_vals = (Bz_function(r_samples, z_val + 1e-7, a_layer, N_o, current, l) - 
                           Bz_function(r_samples, z_val - 1e-7, a_layer, N_o, current, l)) / (2 * 1e-7)
            integrand_matrix[i, :] = dBz_dz_vals * r_samples

        radial_integration = np.trapz(integrand_matrix, r_samples, axis=1)
        double_integral = np.trapz(radial_integration, z_samples)

        F_layer = M_r * 2 * np.pi * double_integral

        B_total += B_avg
        F_total += F_layer

    results.append([current, B_total, abs(F_total)])

    if abs(F_total) >= F_MAX:
        stop_current = current
        break

    current += current_increment

end_time = time.time()
elapsed = end_time - start_time

df = pd.DataFrame(results, columns=['Current_A', 'Weighted_B_Field_T', 'Total_Force_N'])
print(df)
print(f"\nElapsed time: {elapsed:.2f} seconds")
print(f"Stopping current where force >= F_MAX: {stop_current} A")
