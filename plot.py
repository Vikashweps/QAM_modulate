import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from math import erfc

df = pd.read_csv('ber_results.csv', skipinitialspace=True)

v = df['dispers'].values   
v_theory = v[v > 0]
ber_theory = 0.5 * np.array([erfc(1.0 / np.sqrt(2.0 * var)) for var in v_theory])

plt.figure(figsize=(10,6))
plt.plot(v, df['ber'], 'bo-', linewidth=2, markersize=8, label='Simulated BER')
plt.plot(v_theory, ber_theory, 'r--', linewidth=2, label='Theoretical QPSK BER')
plt.xlabel('Noise variance', fontsize=12)
plt.ylabel('Bit Error Rate (BER)', fontsize=12)
plt.title('Bit Error Rate and AWGN Dispers ', fontsize=14)
plt.grid(True, alpha=0.3)
plt.legend()
plt.tight_layout()
plt.savefig('qpsk_ber_with_theory.png', dpi=150)
plt.show()