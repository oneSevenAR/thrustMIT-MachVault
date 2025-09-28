import numpy as np
from mpmath import ellipk, ellippi
import mpmath
from scipy.integrate import dblquad
import time
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

def Bz_function_single(r, z, a, N, I, coil_len, cache):
    z1 = z + coil_len / 2
    z2 = z - coil_len / 2
    prefactor = mu0 * N * I / (2 * np.pi * coil_len)
    return prefactor * (x_elliptic(z2, r, a, cache) - x_elliptic(z1, r, a, cache))

def dBz_dz_function(r, z, a, N, I, coil_len, cache, dz=1e-7):
    """Calculate dBz/dz using numerical differentiation"""
    return (Bz_function_single(r, z + dz, a, N, I, coil_len, cache) - 
            Bz_function_single(r, z - dz, a, N, I, coil_len, cache)) / (2 * dz)

def compute_for_single_case(a_core_cm, l_cm, r_id_cm, r_od_cm):
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

    z_min = z_actual - t_p / 2
    z_max = z_actual + t_p / 2
    M_r = B_r / mu0

    current = current_increment
    F_at_I = None
    
    # Initialize progress bar
    pbar = tqdm(desc="Finding I_max", unit="iter", ncols=100)
    iteration = 0
    
    while True:  # No max current cap, run until reaching F_MAX
        F_total = 0.0
        
        for layer in range(N_l):
            a_layer = a_core + t_w * layer + t_w / 2
            
            # Define integrand function for double integration
            def integrand(z, r):
                return dBz_dz_function(r, z, a_layer, N_o, current, l, cache) * r
            
            # Perform absolute double integration
            double_integral, error = dblquad(
                integrand, 
                r_id, r_od,  # r limits
                lambda r: z_min, lambda r: z_max,  # z limits (constant for each r)
                epsabs=1e-10, epsrel=1e-10
            )
            
            F_layer = M_r * 2 * np.pi * double_integral
            F_total += F_layer
        
        # Update progress bar
        iteration += 1
        pbar.update(1)
        pbar.set_postfix({
            "Current I": f"{current:.2f}A", 
            "F_total": f"{abs(F_total):.2f}N",
            "F_target": f"{F_MAX:.2f}N"
        })
        
        if abs(F_total) >= F_MAX:
            F_at_I = F_total
            break
        current += current_increment
    
    pbar.close()
    return (a_core_cm, l_cm, N_o, N_l, current, F_MAX)

if __name__ == "__main__":
    # Start timing
    start_time = time.time()
    
    # User input for r_id and r_od
    r_id_cm = float(input("Enter inner radius r_id (cm): "))
    r_od_cm = float(input("Enter outer radius r_od (cm): "))

    # Fixed values as requested
    a_core_cm = 1.0
    l_cm = 6
    
    print(f"Computing for a_core = {a_core_cm} cm, l = {l_cm} cm")
    print("Starting computation...")
    
    result = compute_for_single_case(a_core_cm, l_cm, r_id_cm, r_od_cm)
    a_core, l, N_o, N_l, I_max, F_MAX = result
    
    # End timing
    end_time = time.time()
    total_time = end_time - start_time
    
    print(f"\nResults:")
    print(f"Core radius: {a_core} cm")
    print(f"Coil length: {l} cm") 
    print(f"Turns per layer (N_o): {N_o}")
    print(f"Number of layers (N_l): {N_l}")
    print(f"Required current (I_max): {I_max:.3f} A")
    print(f"Target force (F_MAX): {F_MAX:.2f} N")
    print(f"\nTotal execution time: {total_time:.2f} seconds")
