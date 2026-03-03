# /bin/python
from sprtop.sprtop_core import SPRCoreCHA
from sprtop.graph import SPRTopMap


# Cell type labels
CELL_LABELS = {
    0: "",       # empty
    2: "IMC",    # internal memory controller
    3: "UPI",    # UPI link
    4: "PCIe",   # PCIe/CXL
    5: "dis",    # disabled core
}


def format_cell(value, topo_value):
    """Format a single cell from the coremap for display.

    value: from coremap (negative = CHA ID encoded as -(id+1), positive = template type)
    topo_value: from topology grid (1 = active core position)
    """
    if value < 0:
        cha_id = -(value + 1)
        return f"C{cha_id}"
    return CELL_LABELS.get(value, str(value))


def run():
    num_sockets = SPRCoreCHA.get_num_sockets()
    for socket_id in range(num_sockets):
        cha = SPRCoreCHA(socket_id)
        tile_type = cha.get_tile_type()
        num_cores = cha.get_num_cores()
        coremap = cha.get_coremap()
        grid = cha.get_topology_grid()

        # Build header info
        header = [[f"Socket {socket_id}", f"Type: {tile_type}", f"Cores: {num_cores}"]]

        # Build grid display from coremap
        grid_rows = []
        for i, row in enumerate(coremap):
            grid_row = []
            for j, val in enumerate(row):
                topo_val = grid[i][j] if i < len(grid) and j < len(grid[i]) else 0
                grid_row.append(format_cell(val, topo_val))
            grid_rows.append(grid_row)

        # Build core->CHA mapping table
        mappings = []
        for core_id in range(num_cores):
            try:
                cha_id = cha.get_core_cha(core_id)
                mappings.append(f"Core {core_id} -> CHA {cha_id}")
            except IndexError:
                mappings.append(f"Core {core_id} -> ???")

        # Display: pad mapping list to match grid column count
        cols = len(grid_rows[0]) if grid_rows else 3
        map_rows = []
        for i in range(0, len(mappings), cols):
            row = mappings[i:i + cols]
            while len(row) < cols:
                row.append("")
            map_rows.append(row)
        if not map_rows:
            map_rows = [["" for _ in range(cols)]]

        # Pad header to match column count
        for row in header:
            while len(row) < cols:
                row.append("")

        SPRTopMap(header, grid_rows, map_rows).display()
        if socket_id < num_sockets - 1:
            print()  # blank line between sockets


if __name__ == "__main__":
    run()
