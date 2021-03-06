/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * @flow strict-local
 * @format
 */
import type {ComponentType} from 'react';

import AddBox from '@material-ui/icons/AddBox';
import ArrowDownward from '@material-ui/icons/ArrowDownward';
import Check from '@material-ui/icons/Check';
import ChevronLeft from '@material-ui/icons/ChevronLeft';
import ChevronRight from '@material-ui/icons/ChevronRight';
import Clear from '@material-ui/icons/Clear';
import DeleteOutline from '@material-ui/icons/DeleteOutline';
import Edit from '@material-ui/icons/Edit';
import FilterList from '@material-ui/icons/FilterList';
import FirstPage from '@material-ui/icons/FirstPage';
import FormControl from '@material-ui/core/FormControl';
import LastPage from '@material-ui/icons/LastPage';
import MaterialTable from 'material-table';
import Menu from '@material-ui/core/Menu';
import MenuItem from '@material-ui/core/MenuItem';
import MoreVertIcon from '@material-ui/icons/MoreVert';
import OutlinedInput from '@material-ui/core/OutlinedInput';
import Paper from '@material-ui/core/Paper';
import React, {useState} from 'react';
import Remove from '@material-ui/icons/Remove';
import SaveAlt from '@material-ui/icons/SaveAlt';
import Search from '@material-ui/icons/Search';
import Select from '@material-ui/core/Select';

import {CardTitleRow} from './layout/CardTitleRow';
import {forwardRef} from 'react';

const tableIcons = {
  Add: forwardRef((props, ref) => <AddBox {...props} ref={ref} />),
  Check: forwardRef((props, ref) => <Check {...props} ref={ref} />),

  Clear: forwardRef((props, ref) => <Clear {...props} ref={ref} />),
  Edit: forwardRef((props, ref) => <Edit {...props} ref={ref} />),
  Delete: forwardRef((props, ref) => <DeleteOutline {...props} ref={ref} />),

  Export: forwardRef((props, ref) => <SaveAlt {...props} ref={ref} />),
  FirstPage: forwardRef((props, ref) => <FirstPage {...props} ref={ref} />),
  LastPage: forwardRef((props, ref) => <LastPage {...props} ref={ref} />),
  NextPage: forwardRef((props, ref) => <ChevronRight {...props} ref={ref} />),
  PreviousPage: forwardRef((props, ref) => (
    <ChevronLeft {...props} ref={ref} />
  )),
  ResetSearch: forwardRef((props, ref) => <Clear {...props} ref={ref} />),
  Search: forwardRef((props, ref) => <Search {...props} ref={ref} />),
  SortArrow: forwardRef((props, ref) => <ArrowDownward {...props} ref={ref} />),
  ThirdStateCheck: forwardRef((props, ref) => <Remove {...props} ref={ref} />),
  Filter: forwardRef((props, ref) => <FilterList {...props} ref={ref} />),
};

type ActionMenuItems = {
  name: string,
  handleFunc?: () => void,
};

type ColumnType =
  | 'boolean'
  | 'numeric'
  | 'date'
  | 'datetime'
  | 'time'
  | 'currency';

type ActionTableColumn = {
  title: string,
  type?: ColumnType,
  field: string,
};

type ActionTableOptions = {
  actionsColumnIndex: number,
  pageSizeOptions: Array<number>,
};

type ActionOrderType = {
  field: string,
  title: string,
  tableData: {},
};

export type ActionQuery = {
  filters: Array<string>,
  orderBy: ActionOrderType,
  orderDirection: string,
  page: number,
  pageSize: number,
  search: string,
  totalCount: number,
};

export type ActionTableProps<T> = {
  titleIcon?: ComponentType<SvgIconExports>,
  tableRef?: {},
  editable?: {},
  title?: string,
  handleCurrRow?: T => void,
  columns: Array<ActionTableColumn>,
  menuItems?: Array<ActionMenuItems>,
  actions?: Array<{}>,
  data: Array<T> | (ActionQuery => {}),
  options: ActionTableOptions,
};

export function PaperComponent(props: {}) {
  return <Paper {...props} elevation={0} />;
}

type SelectProps = {
  content: Array<string>,
  defaultValue?: string,
  value: string,
  onChange: string => void,
};

export function SelectEditComponent(props: SelectProps) {
  if (props.value === undefined || props.value === null) {
    if (props.defaultValue !== undefined) {
      props.onChange(props.defaultValue);
      return null;
    }
  }
  return (
    <FormControl>
      <Select
        value={props.value}
        onChange={({target}) => props.onChange(target.value)}
        input={<OutlinedInput />}>
        {props.content.map((k: string, idx: number) => (
          <MenuItem key={idx} value={k}>
            {k}
          </MenuItem>
        ))}
      </Select>
    </FormControl>
  );
}

export default function ActionTable<T>(props: ActionTableProps<T>) {
  const actionTableJSX = [];
  const [anchorEl, setAnchorEl] = useState(null);
  const handleClick = (event, row: T) => {
    setAnchorEl(event.currentTarget);
    if (props.handleCurrRow) {
      props.handleCurrRow(row);
    }
  };

  const handleClose = () => {
    setAnchorEl(null);
  };
  if (props.titleIcon) {
    const TitleIcon = props.titleIcon;
    actionTableJSX.push(
      <CardTitleRow
        key="title"
        icon={TitleIcon}
        label={`${props.title || ''} (${props.data.length})`}
      />,
    );
  }
  if (props.menuItems) {
    const menuItems: Array<ActionMenuItems> = props.menuItems;
    actionTableJSX.push(
      <Menu
        key="menu"
        id="actions-menu"
        data-testid="actions-menu"
        anchorEl={anchorEl}
        keepMounted
        open={Boolean(anchorEl)}
        onClose={handleClose}>
        {menuItems.map(item => (
          <MenuItem
            key={item.name}
            onClick={() => {
              if (item.handleFunc) {
                item.handleFunc();
              }
            }}>
            {item.name}
          </MenuItem>
        ))}
      </Menu>,
    );
  }
  return (
    <>
      {actionTableJSX}
      <MaterialTable
        tableRef={props.tableRef}
        editable={props.editable}
        components={{
          Container: PaperComponent,
        }}
        title=""
        columns={props.columns}
        icons={tableIcons}
        data={props.data}
        actions={
          props.menuItems
            ? [
                ...(props.actions ? props.actions : []),
                {
                  icon: () => <MoreVertIcon />,
                  tooltip: 'Actions',
                  onClick: handleClick,
                },
              ]
            : props.actions
        }
        options={props.options}
      />
    </>
  );
}
