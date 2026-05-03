import numpy as np
import plotly.graph_objects as go

# Create time array: 0 to 1.999 seconds in 1ms increments (2000 points)
time = np.arange(0, 2000) * 1e-3

data = np.fromfile('measurements/angle_meas13.bin', dtype=np.float32)

# Reshape into 3 arrays of 2000 points each
if len(data) >= 6000:
    angles = data[:6000].reshape(3, 2000)
    angle1, angle2, angle3 = angles[0], angles[1], angles[2]
else:
    angle1 = data[0:2000]
    angle2 = data[2000:4000]
    angle3 = data[4000:6000]

fig = go.Figure()

fig.add_trace(go.Scatter(
    x=time, y=angle1,
    mode='lines',
    name='Angle 1',
    hovertemplate='Time: %{x:.3f}s<br>Angle 1: %{y:.4f}<extra></extra>'
))

fig.add_trace(go.Scatter(
    x=time, y=angle2,
    mode='lines',
    name='Angle 2',
    hovertemplate='Time: %{x:.3f}s<br>Angle 2: %{y:.4f}<extra></extra>'
))

fig.add_trace(go.Scatter(
    x=time, y=angle3,
    mode='lines',
    name='Angle 3',
    hovertemplate='Time: %{x:.3f}s<br>Angle 3: %{y:.4f}<extra></extra>'
))

fig.update_layout(
    title='Angle Measurements vs Time',
    xaxis_title='Time (seconds)',
    yaxis_title='Angle',
    hovermode='x unified',   # shows all 3 values at the same x on hover
    legend=dict(x=1, xanchor='right'),
    template='plotly_white'
)

fig.show()