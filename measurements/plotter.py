import plotly.graph_objects as go
import numpy as np

x, y = np.random.rand(50), np.random.rand(50)

fig = go.Figure(go.Scatter(
    x=x, y=y, mode='markers',
    hovertemplate="x: %{x:.2f}<br>y: %{y:.2f}<extra></extra>"
))
fig.show()