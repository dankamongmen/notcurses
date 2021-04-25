# SPDX-License-Identifier: Apache-2.0

# Copyright 2020 igo95862

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

# http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


from time import sleep

from notcurses import Notcurses

nc = Notcurses()

stdplane = nc.stdplane()

test_str = '''Sapiente quaerat expedita repellendus ea quae. Ut enim natus iure laborum. Assumenda sed placeat provident similique cum quidem. Sit voluptas facilis vitae culpa asperiores eos neque.
Aspernatur rerum quae minus natus. Vero autem suscipit nisi eligendi dolorum sed vero. Illum odio repudiandae odit in voluptas reiciendis amet.
Sunt ea hic repudiandae beatae. Nisi asperiores aut commodi dolorem itaque illum sunt eum. Aperiam illo ratione in. Eaque perspiciatis repellat minima culpa et consequatur voluptatem voluptas.
Laboriosam expedita ut enim velit occaecati qui neque. Et voluptatem eligendi harum sed ducimus enim culpa. Quia expedita distinctio provident qui et dolorem placeat. Provident aut corporis laudantium quo.
Dolores quaerat sint dolorum. Corrupti temporibus nam corrupti. Iusto non perspiciatis et quisquam minima nesciunt quia esse.
'''

wr = stdplane.puttext(-1, 0, test_str)

nc.render()

sleep(3)
